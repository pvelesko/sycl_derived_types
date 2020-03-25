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
class Particle {
	public:
	T pos[3]; // coordinate(x, y, z)
}
</code></pre>
<h3 id="particleaos---making-aos-data-structures-sycl-compliant">ParticleAoS - Making AoS data structures SYCL compliant</h3>
<p>Array of Structures implementation for representing all particles.</p>
<pre><code>template&lt;class T&gt;
class ParticleAoS {
  public:
  Particle&lt;T&gt;* ptls;
  ParticleAoS(int n) {
	ptls = static_cast&lt;Particle&lt;T&gt;*&gt;(
		malloc(n * sizeof(Particle&lt;T&gt;)));
 };
  ~ParticleAoS&lt;T&gt;() {
    free(ptls);
  }
 };
</code></pre>
<p>First, ptls pointer needs to be made private. You will need a function to</p>
<h4 id="host-operations-on-aos-types">Host Operations on AoS types</h4>
<pre><code>template&lt;class T&gt;
void ptl_incr(ParticleAoS&lt;T&gt; p) {
  for (int i = 0; i &lt; p.ptls.size(); i++) {
    p.ptls.pos[0] = 1;
    p.ptls.pos[1] = 2;
    p.ptls.pos[2] = 3;
  }
}
</code></pre>
<h2 id="function-nests">Function nests</h2>
<p>When trying to port a chain of function calls, it’s better to start from outer loop and go down.</p>
<p>Inner most functions can take pointer arguments as they will be inlined into outer.<br>
Outer must take accessor arguments<br>
specify abs, min, max to std:: namespace<br>
first issue - no support  beying 3d buffers</p>
<h2 id="dealing-with-classes">dealing with classes</h2>
<p>push_c gets offloaded but uses classes downstream<br>
classes can’t be trivially copied.<br>
classes contain buffers that have already been allocated.<br>
Rebuild the classes inside the region?</p>
<blockquote>
<p>Written with <a href="https://stackedit.io/">StackEdit</a>.</p>
</blockquote>
<p>when creating buffer acessors,  will default to host_target, unless you pass cgh as an argument</p>

