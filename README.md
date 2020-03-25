---


---

<h1 id="sycl-compliant-data-structures-with-usm">SYCL Compliant Data Structures with USM</h1>
<h3 id="sycl-kernel-requirements">SYCL Kernel Requirements</h3>
<p>These requirements must be met for SYCL kernels:</p>
<ol>
<li>Data structures must be trivially copyable
<blockquote>
<p>Default copy constructor and default destructor<br>
<a href="https://en.cppreference.com/w/cpp/types/is_trivially_copyable">std::is_trivially_copyable</a></p>
</blockquote>
</li>
<li>Data structures adhere to standard layout
<blockquote>
<p>class members declared private, no virtual functions, etc…<br>
<a href="https://en.cppreference.com/w/cpp/types/is_standard_layout">https://en.cppreference.com/w/cpp/types/is_standard_layout</a></p>
</blockquote>
</li>
</ol>
<h4 id="checking-if-data-structure-meets-requirements">Checking if data structure meets requirements</h4>
<p>You can check if this class is standard layout by :<br>
<code>std::cout &lt;&lt; std::is_standard_layout&lt;ParticleAoS&lt;T&gt;&gt;::value;</code><br>
and for testing trivially copyable requirement:<br>
<code>std::cout &lt;&lt; std::is_trivially_copyable&lt;ParticleAoS&lt;T&gt;&gt;::value;</code></p>
<h2 id="original-implementation">Original Implementation</h2>
<h3 id="particle">Particle</h3>
<p>Single instance of a particle-like object.  Very simple data structure, can be passed to SYCL kernels.</p>
<pre><code>template&lt;class T&gt;
struct Particle {
	T pos[3]; // coordinate(x, y, z)
}
</code></pre>
<h3 id="particleaos---making-aos-data-structures-sycl-compliant">ParticleAoS - Making AoS data structures SYCL compliant</h3>
<p>Original AoS implementation for representing all particles. I could use <code>vector&lt;Particle&lt;T&gt;&gt;</code> and provide the USM allocator as well.</p>
<pre><code>template&lt;class T&gt;
class ParticleAoS {
  Particle&lt;T&gt;* ptls;
  public:
  ParticleAoS(int n) {
	ptls = static_cast&lt;Particle&lt;T&gt;*&gt;(
		malloc(n * sizeof(Particle&lt;T&gt;)));
 };
  ~ParticleAoS&lt;T&gt;() {
    free(ptls);
  }
  Particle&lt;T&gt;* data() const {
    return ptls;
   }

 };
</code></pre>
<h3 id="example-function-operating-on-aos">Example function operating on AoS</h3>
<pre><code>template&lt;class T&gt;
void ptl_incr(ParticleAoS&lt;T&gt; p) {
  for (int i = 0; i &lt; p.ptls.size(); i++) {
    p.data()[i].pos[0] = 1;
    p.data()[i].pos[1] = 2;
    p.data()[i].pos[2] = 3;
  }
}
</code></pre>
<pre><code>template&lt;class T&gt;
void ptl_incr(ParticleAoS&lt;T&gt; p, int i) {
  p.data()[i].pos[0] = 1;
  p.data()[i].pos[1] = 2;
  p.data()[i].pos[2] = 3;
}
</code></pre>
<p>Whether the compute-intensive loop is inside the function or outside depends on the overall structure of the code but both cases are common but the latter more naturally maps to <code>parallel_for</code> for this example.</p>
<h2 id="sycl-usm-compatible-implementation">SYCL USM Compatible Implementation</h2>
<pre><code>template&lt;class T&gt;
class ParticleAoS {
  private:
  int* ref; // reference counter for USM
  Particle&lt;T&gt;* ptls;
  
  public:
  ParticleAoS(int n) {
    ptls = static_cast&lt;Particle&lt;T&gt;*&gt;(malloc_shared(n * sizeof(Particle&lt;T&gt;), q));
    ref = static_cast&lt;int*&gt;(malloc_shared(sizeof(int), q));
    *ref = 1;
  };
  Particle&lt;T&gt;* data() const {
    return ptls;
  };

  /* This section gets compiled for the host only
    Allows for proper deallocation of USM */
#ifndef \_\_SYCL_DEVICE_ONLY\_\_
  ParticleAoS(const ParticleAoS&lt;T&gt;&amp; other) {
    ptls = other.ptls;
    ref = other.ref;
    (*ref)++;
  };

  ~ParticleAoS() {
    (*ref)--;
    if (*ref == 0) {
      free(ptls, q);
      free(ref, q);
      std::cout &lt;&lt; "USM deallocated" &lt;&lt; std::endl;
    }
  };
#endif

};

</code></pre>
<h3 id="constructor">Constructor</h3>
<p>To meet the layout requirement we make <code>ptls</code> pointer private, along with <code>data()</code> function to return the pointer.</p>
<p>Porting the constructor is trivial, just replace <code>malloc</code> with <code>malloc_shared</code> and pass <code>queue</code> object.<br>
Most often I use globally declared <code>queue</code> objects that are managed from a separate header. I just <code>#import "Util.hpp"</code> which declares global <code>queue</code>, <code>device</code>, and <code>context</code> objects and then call <code>init()</code>which initializes them.</p>
<h3 id="destructor">Destructor</h3>
<p>Since we are using malloc to allocate memory, we also need to free that memory:<br>
<code>free(ptr, q);</code><br>
This is normally done in the destructor. There is a problem, however.<br>
SYCL requires data entering the kernel to be passed by copy [=] rather than reference [&amp;].  This means that the ParticleAoS object will be copied into the Kernel and its destructor will be called upon exit. This will result in a double free error.</p>
<p>This can be solved by reference counting our pointer and implementing custom copy constructor and destructor for ParticleAoS <em>but only for the host</em> because we can’t override the defaults for SYCL Kernel code without violating is_standard_layout requirement.</p>
<blockquote>
<p>shared_ptr might also with if provided a USM allocator but at the time of writing such use case fails to compile.</p>
</blockquote>
<p><code>__SYCL_HOST_ONLY__</code> macro returns if the code is to be run on the  device.<br>
<code>int* ref</code> will keep track of reference count.</p>
<pre><code>#ifndef __SYCL_DEVICE_ONLY__
   ParticleAoS(const ParticleAoS&lt;T&gt;&amp; other) {
     ptls = other.ptls;
     ref = other.ref;
     (*ref)++;
   };

   ~ParticleAoS() {
     (*ref)--;
     if (*ref == 0) {
       free(ptls, q);
       free(ref, q);
       std::cout &lt;&lt; "USM deallocated" &lt;&lt; std::endl;
     }
   };
#endif
</code></pre>
<h2 id="using-sycl-with-aos">Using SYCL with AoS</h2>
<p>Finally we can use a function and AoS datatypes inside our SYCL kernels. Make sure the tasks completes before reading the data.</p>
<pre><code>   ParticleAoS&lt;float&gt; particles(N);
   q.submit([&amp;](handler&amp; cgh) { // q scope
     cgh.parallel_for(range&lt;1&gt;(N), [=](id&lt;1&gt; idx) mutable { // needs to be mutable, otherwise particles are const
       ptl_incr(particles, idx, N);
     } ); // end task scope
   } ); // end q scope
   q.wait();
</code></pre>

