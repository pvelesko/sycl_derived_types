---


---

<h2 id="sycl-compliant-data-structures">SYCL Compliant Data Structures</h2>
<p>There are 2 main requirements for SYCL kernels:</p>
<ol>
<li>Must be trivially copyable<br>
Default copy constructor and default destructor</li>
<li>Must adhere to standard layout<br>
Ensure that all variables are private</li>
</ol>
<h3 id="particle">Particle</h3>
<p>Single instance of a particle-like object.  Very simple data structure, can be passed to SYCL kernels.</p>
<pre><code>template&lt;class T&gt;
struct Particle {
	T pos[3]; // coordinate(x, y, z)
}
</code></pre>
<h3 id="particleaos---making-aos-data-structures-sycl-compliant">ParticleAoS - Making AoS data structures SYCL compliant</h3>
<p>Array of Structures implementation for representing all particles. I could use <code>vector&lt;Particle&lt;T&gt;&gt;</code> but, I will need to override copy constructor and destructor as you will see later.</p>
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
 };
</code></pre>
<h4 id="checking-if-data-structure-meets-requirements">Checking if data structure meets requirements</h4>
<p>You can check if this class is standard layout by :<br>
<code>std::cout &lt;&lt; std::is_standard_layout&lt;ParticleAoS&lt;T&gt;&gt;::value;</code><br>
and for testing trivially copyable requirement:<br>
<code>std::cout &lt;&lt; std::is_trivially_copyable&lt;ParticleAoS&lt;T&gt;&gt;::value;</code></p>
<p>To meet the layout requirement we make private <code>ptls</code> pointer, along with <code>data()</code> function to return the pointer.</p>
<h3 id="constructor">Constructor</h3>
<p>Porting the constructor is trivial, just replace <code>malloc</code> with <code>malloc_shared</code> and pass <code>queue</code> object.<br>
Most often I use globally declared <code>queue</code> objects that are managed from a separate header.</p>
<h3 id="destructor">Destructor</h3>
<p>Since we are using malloc to allocate memory, we also need to free that memory:<br>
<code>free(ptr, q);</code><br>
This is normally done in the destructor. There is a problem, however.<br>
SYCL requires data entering the kernel to be passed by copy [=] rather than reference [&amp;].  This means that the ParticleAoS object will be copied into the Kernel and its destructor will be called upon exit. This will result in a double free error.</p>
<p>This can be solved by reference counting our pointer and implementing custom copy constructor and destructor for ParticleAoS <em>but only for the host</em> because we can’t override the defaults for SYCL Kernel code without violating is_standard_layout requirement.</p>
<p><code>__SYCL_HOST_ONLY__</code> macro returns if the code is to be run on the host or the device.<br>
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
<h4 id="example-function-using-aos">Example function using AoS</h4>
<pre><code>template&lt;class T&gt;
void ptl_incr(ParticleAoS&lt;T&gt; p) {
  for (int i = 0; i &lt; p.ptls.size(); i++) {
    p.data()[i].pos[0] = 1;
    p.data()[i].pos[1] = 2;
    p.data()[i].pos[2] = 3;
  }
}
</code></pre>
<p>A common pattern in HPC but if we want to do this in parallel on the GPU we need to get rid of the inner loop and add an integer argument that will be provided by parallel_for.</p>
<pre><code>template&lt;class T&gt;
void ptl_incr(ParticleAoS&lt;T&gt; p, int i) {
  p.data()[i].pos[0] = 1;
  p.data()[i].pos[1] = 2;
  p.data()[i].pos[2] = 3;
}
</code></pre>
<h2 id="using-sycl-with-aos">Using SYCL with AoS</h2>
<pre><code>   ParticleAoS&lt;float&gt; particles(N);
   q.submit([&amp;](handler&amp; cgh) { // q scope
     cgh.parallel_for(range&lt;1&gt;(N), [=](id&lt;1&gt; idx) mutable { // needs to be mutable, otherwise particles are const
       ptl_incr(particles, idx, N);
     } ); // end task scope
   } ); // end q scope
   q.wait();
</code></pre>

