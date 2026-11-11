#pragma once
// GLSL for the main menu: an animated space scene, the BEYOND logo and the rounded widgets.
// Everything is drawn by shaders, so the menu needs no image files.

// Fullscreen quad with no vertex buffer: the corners come from gl_VertexID.
static const char* VS_QUAD = R"(#version 330 core
out vec2 vUV;
void main(){ vec2 q=vec2(float(gl_VertexID&1),float((gl_VertexID>>1)&1)); vUV=q; gl_Position=vec4(q*2.0-1.0,0.0,1.0); })";

// Nebula: slow, soft and expensive, so it is drawn into a small texture and scaled up.
static const char* FS_NEBULA = R"(#version 330 core
in vec2 vUV; out vec4 oC; uniform float uT, uAsp;
float h21(vec2 p){ vec3 q=fract(p.xyx*0.1031); q+=dot(q,q.yzx+33.33); return fract((q.x+q.y)*q.z); }
float vn(vec2 p){ vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
  return mix(mix(h21(i),h21(i+vec2(1.0,0.0)),f.x),mix(h21(i+vec2(0.0,1.0)),h21(i+vec2(1.0,1.0)),f.x),f.y); }
float fbm(vec2 p){ float a=0.5, s=0.0; mat2 R=mat2(0.8,0.6,-0.6,0.8); for(int i=0;i<6;i++){ s+=a*vn(p); p=R*p*2.03+vec2(17.1,9.2); a*=0.5; } return s; }
void main(){
  vec2 p=(vUV-0.5)*vec2(uAsp,1.0); float t=uT*0.011;
  vec2 q=vec2(fbm(p*1.5+vec2(0.0,t)),fbm(p*1.5+vec2(5.2,1.3-t)));
  vec2 r=vec2(fbm(p*1.5+3.4*q+vec2(1.7,9.2)+t*0.6),fbm(p*1.5+3.4*q+vec2(8.3,2.8)-t*0.4));
  float f=fbm(p*1.5+3.6*r);
  float across=dot(p,normalize(vec2(0.52,-0.85)))+(q.x-0.5)*0.75;
  float band=exp(-across*across*5.5);
  float cloud=pow(clamp(f*1.45-0.22,0.0,1.0),2.4);
  vec3 deep=vec3(0.0022,0.0035,0.0120)+vec3(0.0015,0.0030,0.0090)*(1.0-vUV.y);
  vec3 cool=mix(vec3(0.020,0.170,0.380),vec3(0.360,0.060,0.430),smoothstep(0.30,0.72,q.y));
  vec3 col=deep+cool*cloud*(0.035+0.62*band);
  col+=vec3(1.00,0.42,0.15)*pow(clamp(r.x*1.5-0.42,0.0,1.0),3.0)*band*0.30;
  col+=vec3(0.30,0.72,0.86)*pow(clamp(r.y*1.5-0.48,0.0,1.0),3.5)*band*0.20;
  float dust=smoothstep(0.42,0.72,fbm(p*3.2+r*2.4+vec2(3.1,7.7)));
  col*=1.0-0.72*dust*band;
  oC=vec4(col,1.0); })";

// The scene: stars, a rising sun behind a living planet, a ringed giant, a moon, and a rocket leaving.
static const char* FS_SPACE = R"(#version 330 core
in vec2 vUV; out vec4 oC; uniform vec2 uRes, uMouse; uniform float uT; uniform sampler2D uNeb;
const vec3 SUN=vec3(-0.608,0.618,-0.498);
float h21(vec2 p){ vec3 q=fract(p.xyx*0.1031); q+=dot(q,q.yzx+33.33); return fract((q.x+q.y)*q.z); }
float h31(vec3 p){ p=fract(p*0.1031); p+=dot(p,p.zyx+31.32); return fract((p.x+p.y)*p.z); }
float n3(vec3 x){ vec3 i=floor(x), f=fract(x); f=f*f*(3.0-2.0*f);
  return mix(mix(mix(h31(i),h31(i+vec3(1.0,0.0,0.0)),f.x),mix(h31(i+vec3(0.0,1.0,0.0)),h31(i+vec3(1.0,1.0,0.0)),f.x),f.y),
             mix(mix(h31(i+vec3(0.0,0.0,1.0)),h31(i+vec3(1.0,0.0,1.0)),f.x),mix(h31(i+vec3(0.0,1.0,1.0)),h31(i+vec3(1.0,1.0,1.0)),f.x),f.y),f.z); }
// each octave is rotated, which hides the axis-aligned grid that plain value noise shows up close
float fbm3(vec3 p,int oct){ float a=0.5, s=0.0; mat3 M=mat3(0.0,0.8,0.6,-0.8,0.36,-0.48,-0.6,-0.48,0.64); for(int i=0;i<oct;i++){ s+=a*n3(p); p=M*p*2.02+vec3(13.1,7.3,5.7); a*=0.5; } return s; }
vec3 rotY(vec3 v,float a){ float c=cos(a), s=sin(a); return vec3(c*v.x+s*v.z,v.y,-s*v.x+c*v.z); }
vec3 rotX(vec3 v,float a){ float c=cos(a), s=sin(a); return vec3(v.x,c*v.y-s*v.z,s*v.y+c*v.z); }

vec3 starLayer(vec2 p,float sc,float dens,float seed,float gain){
  vec2 g=floor(p*sc), f=fract(p*sc); float h=h21(g+seed); if(h>dens) return vec3(0.0);
  vec2 o=vec2(h21(g+seed+3.1),h21(g+seed+7.7))*0.8+0.1; float d=length(f-o)/sc, b=h21(g+seed+11.3);
  float r=max((0.55+1.25*b*b)/1080.0,0.8/uRes.y), tw=0.78+0.22*sin(uT*(0.5+b*2.4)+h*70.0);
  vec3 c=mix(vec3(1.0,0.80,0.62),vec3(0.68,0.83,1.0),h21(g+seed+19.3));
  return c*exp(-d*d/(r*r))*(0.012+1.5*b*b*b*b*b)*tw*gain; }
vec3 flareStars(vec2 p){
  vec2 g=floor(p*7.0), f=fract(p*7.0); float h=h21(g+71.0); if(h>0.10) return vec3(0.0);
  vec2 o=vec2(h21(g+74.1),h21(g+78.7))*0.7+0.15; vec2 d=(f-o)/7.0; float l=length(d), tw=0.8+0.2*sin(uT*0.7+h*90.0);
  float glow=exp(-l*l/0.0000022)+0.10*exp(-l*l/0.00018), spike=exp(-abs(d.x)*520.0)*exp(-abs(d.y)*52.0)+exp(-abs(d.y)*520.0)*exp(-abs(d.x)*52.0);
  return mix(vec3(1.0,0.86,0.72),vec3(0.72,0.86,1.0),h21(g+83.3))*(glow*1.5+spike*0.20)*tw; }

float cityDots(vec3 q,float sc,float keep){ vec3 g=floor(q*sc), f=fract(q*sc)-0.5; float h=h31(g); return h<keep?0.0:exp(-dot(f,f)*34.0)*(0.35+h); }
// hero planet: oceans, continents, clouds, city lights on the night side, and a glowing atmosphere
vec3 hero(vec2 d,out float inner){
  float r2=dot(d,d); vec3 n=vec3(d,sqrt(max(1.0-r2,0.0)));
  vec3 q=rotY(rotX(n,0.42),uT*0.0055);
  float warp=fbm3(q*2.6+11.0,4), land=fbm3(q*4.4+warp*1.9,6), sea=0.49;
  float lm=smoothstep(sea,sea+0.012,land), hgt=clamp((land-sea)*4.2,0.0,1.0), lat=abs(q.y);
  vec3 ocean=mix(vec3(0.002,0.014,0.052),vec3(0.004,0.070,0.135),smoothstep(sea-0.10,sea,land));
  vec3 ground=mix(vec3(0.026,0.082,0.026),vec3(0.135,0.105,0.050),smoothstep(0.12,0.70,hgt+fbm3(q*14.0,3)*0.22-0.10));
  ground=mix(ground,vec3(0.34,0.32,0.31),smoothstep(0.62,0.92,hgt));
  vec3 alb=mix(ocean,ground,lm); float ice=smoothstep(0.76,0.90,lat+(warp-0.5)*0.22); alb=mix(alb,vec3(0.78,0.84,0.90),ice);
  vec3 cq=rotY(q,uT*0.0035+1.7); float cw=fbm3(cq*2.4+5.0,4), cl=smoothstep(0.50,0.69,fbm3(vec3(cq.x,cq.y*1.7,cq.z)*6.4+cw*2.6,6));
  float ndl=dot(n,SUN), day=smoothstep(-0.06,0.34,ndl), twi=exp(-ndl*ndl*42.0);
  vec3 sun=vec3(1.62,1.44,1.22);
  vec3 col=alb*sun*day;
  vec3 hv=normalize(SUN+vec3(0.0,0.0,1.0)); col+=(1.0-lm)*(1.0-ice)*(1.0-cl)*vec3(1.2,0.95,0.7)*pow(max(dot(n,hv),0.0),120.0)*0.75*day;
  float night=(1.0-smoothstep(-0.22,0.02,ndl)), towns=(cityDots(q,190.0,0.60)+cityDots(q,92.0,0.86)*1.4)*smoothstep(0.47,0.62,fbm3(q*8.0+2.0,4));
  col+=vec3(1.0,0.62,0.26)*towns*lm*(1.0-ice)*night*(1.0-cl*0.9)*1.15;
  vec3 cloudCol=mix(vec3(0.004,0.005,0.010),vec3(1.02,1.0,0.97)*sun*0.56,day)+vec3(0.95,0.30,0.10)*twi*0.30;
  col=mix(col,cloudCol,cl*0.88);
  col+=vec3(0.95,0.34,0.10)*twi*0.10;
  float rim=pow(clamp(1.0-n.z,0.0,1.0),3.2), litRim=smoothstep(-0.32,0.55,ndl);
  col+=mix(vec3(0.95,0.42,0.18),vec3(0.20,0.50,1.05),smoothstep(-0.05,0.45,ndl))*rim*litRim*1.1;
  inner=rim; return col; }

vec3 giant(vec2 p,vec2 c,float r,out float mask){
  vec2 d=(p-c)/r; float a=-0.40, ca=cos(a), sa=sin(a); vec2 t=vec2(ca*d.x-sa*d.y,sa*d.x+ca*d.y);
  float pr=length(d), edge=1.5/(uRes.y*r), body=(1.0-smoothstep(1.0-edge,1.0+edge,pr));
  float rr=length(vec2(t.x,t.y/0.27)), band=0.55+0.45*sin(rr*38.0+sin(rr*11.0)*3.0), ring=smoothstep(1.36,1.42,rr)*(1.0-smoothstep(2.26,2.34,rr))*band;
  ring*=1.0-0.86*smoothstep(1.86,1.88,rr)*(1.0-smoothstep(1.95,1.97,rr));
  vec3 ringCol=mix(vec3(0.46,0.34,0.24),vec3(0.70,0.60,0.46),band)*0.30; float shadow=smoothstep(0.86,1.05,length(vec2(t.x-0.28,t.y)));
  vec3 col=ringCol*ring*shadow; float cov=ring*0.86;
  if(pr<1.0+edge){ vec3 n=vec3(d,sqrt(max(1.0-pr*pr,0.0))); vec3 m=vec3(t,n.z);
    float w=fbm3(vec3(m.x*2.5,m.y*8.0,uT*0.008),4), bands=0.5+0.5*sin(m.y*15.0+w*6.5);
    vec3 alb=mix(vec3(0.30,0.13,0.07),vec3(0.62,0.42,0.24),bands); float ndl=max(dot(n,normalize(vec3(-0.70,0.56,0.44))),0.0);
    vec3 pc=alb*(ndl*1.25+0.002)+vec3(0.65,0.36,0.20)*pow(1.0-n.z,3.0)*ndl*0.6;
    float front=step(t.y,0.0)*ring*0.92; col=mix(mix(col,pc,body),col,front); cov=max(body,front); }
  mask=cov; return col; }

vec3 moon(vec2 p,vec2 c,float r,out float mask){
  vec2 d=(p-c)/r; float pr=length(d), edge=1.5/(uRes.y*r); mask=(1.0-smoothstep(1.0-edge,1.0+edge,pr)); if(pr>1.0+edge) return vec3(0.0);
  vec3 n=vec3(d,sqrt(max(1.0-pr*pr,0.0))); float k=fbm3(n*5.0+4.0,5);
  return vec3(0.55,0.54,0.56)*(0.45+k)*smoothstep(-0.05,0.55,dot(n,SUN))*1.5; }

float seg(vec2 p,vec2 a,vec2 b,out float along){ vec2 ba=b-a; float h=clamp(dot(p-a,ba)/dot(ba,ba),0.0,1.0); along=h; return length(p-a-ba*h); }

void main(){
  vec2 fc=gl_FragCoord.xy; float asp=uRes.x/uRes.y; vec2 p=(fc-0.5*uRes)/uRes.y; float px=1.0/uRes.y;
  vec2 pStar=p+uMouse*0.006, pFar=p+uMouse*0.012, pNear=p+uMouse*0.024;
  vec2 nuv=0.5+(vUV-0.5)*0.94+uMouse*0.006;
  vec3 col=texture(uNeb,nuv).rgb;
  col+=starLayer(pStar,170.0,0.34,1.0,0.60)+starLayer(pStar,82.0,0.36,23.0,0.95)+starLayer(pStar,34.0,0.26,47.0,1.5)+flareStars(pStar);
  // ringed giant and its moon, far away
  float mk; vec2 gc=vec2(min(asp*0.5-0.60,0.32),0.285); vec3 gi=giant(pFar,gc,0.056,mk); col=mix(col,gi,mk);
  gi=moon(pFar,gc+vec2(0.20,-0.09),0.0105,mk); col=mix(col,gi,mk);
  // rocket leaving the planet: a warm head and a long fading trail (one crossing every 52 seconds)
  float cyc=fract(uT/52.0), ease=cyc*cyc*(3.0-2.0*cyc); vec2 ra=vec2(asp*0.5-0.46,-0.02), rb=vec2(asp*0.5+0.06,0.53);
  vec2 head=mix(ra,rb,ease), tail=mix(ra,rb,max(ease-0.20,0.0)); float al, sd=seg(pFar,tail,head,al), vis=smoothstep(0.0,0.03,cyc)*(1.0-smoothstep(0.93,1.0,cyc));
  col+=vec3(0.45,0.70,1.0)*exp(-sd/0.0016)*al*al*0.62*vis;
  float hd=length(pFar-head); col+=(vec3(1.0,0.72,0.38)*exp(-hd*hd/0.000012)*3.2+vec3(1.0,0.45,0.18)*exp(-hd/0.012)*0.16)*vis;
  // hero planet with its sunrise
  float R=0.86; vec2 C=vec2(asp*0.5-0.08,-0.74); vec2 rel=(pNear-C); float dist=length(rel), dp=dist-R, body=(1.0-smoothstep(-px*1.4,px*1.4,dp));
  vec2 sdir=normalize(SUN.xy); float facing=dot(rel/max(dist,0.0001),sdir), arc=pow(clamp(facing*0.5+0.5,0.0,1.0),3.5);
  vec3 sunGlow=vec3(1.0,0.66,0.38), airGlow=vec3(0.16,0.42,1.0);
  if(dp>0.0){ col+=mix(airGlow,sunGlow,smoothstep(0.78,0.99,facing))*(exp(-dp/0.010)*0.85+exp(-dp/0.05)*0.26+exp(-dp/0.20)*0.035)*arc*1.25; }
  if(body>0.0){ float inner; vec3 hc=hero(rel/R,inner); col=mix(col,hc,body); }
  // the sun, sitting just on the planet's edge
  vec2 sp=C+sdir*(R+0.002); vec2 ds=pNear-sp; float sl=length(ds), an=atan(ds.y,ds.x);
  float rays=0.55+0.45*sin(an*9.0+uT*0.05)*sin(an*23.0-uT*0.03);
  col+=vec3(1.0,0.86,0.68)*(0.00062/(sl*sl+0.00016));
  col+=vec3(1.0,0.56,0.27)*exp(-sl/0.075)*(0.20+0.26*rays)+vec3(0.9,0.42,0.22)*exp(-sl/0.32)*0.050;
  col+=vec3(0.65,0.78,1.0)*exp(-abs(ds.y)/0.0026)*exp(-abs(ds.x)/0.34)*0.42;
  vec2 gdir=(vec2(0.0)-sp); for(int i=1;i<=3;i++){ float k=float(i)*0.42; vec2 g=sp+gdir*k; float rg=0.018+0.016*float(i), dg=length(p-g);
    col+=mix(vec3(0.20,0.45,0.80),vec3(0.80,0.42,0.20),float(i)/3.0)*(1.0-smoothstep(rg*0.72,rg,dg))*0.012; }
  // keep the left side calm so the menu stays readable, then finish the image
  vec2 vg=vUV-0.5; col*=1.0-0.62*smoothstep(0.26,0.85,length(vg*vec2(0.86,1.0)));
  col*=mix(0.70,1.0,smoothstep(0.02,0.46,vUV.x));
  col=1.0-exp(-col*1.32); col=pow(col,vec3(1.0/2.2));
  col+=(h21(fc+fract(uT)*61.0)-0.5)/255.0;
  oC=vec4(col,1.0); })";

// Rounded panels, buttons and input fields, with an anti-aliased edge and an optional outer glow.
static const char* VS_WIDGET = R"(#version 330 core
uniform vec4 uR; uniform vec2 uScr; uniform float uPad; out vec2 vP;
void main(){ vec2 q=vec2(float(gl_VertexID&1),float((gl_VertexID>>1)&1)); vec2 p=uR.xy-vec2(uPad)+q*(uR.zw+vec2(2.0*uPad)); vP=p;
  gl_Position=vec4(p.x*2.0/uScr.x-1.0,1.0-p.y*2.0/uScr.y,0.0,1.0); })";
static const char* FS_WIDGET = R"(#version 330 core
in vec2 vP; out vec4 oC; uniform vec4 uR,uTop,uBot,uEdge,uGlow; uniform float uRad,uEW,uGR;
vec4 over(vec4 a,vec4 b){ float o=a.a+b.a*(1.0-a.a); if(o<=0.0) return vec4(0.0); return vec4((a.rgb*a.a+b.rgb*b.a*(1.0-a.a))/o,o); }
void main(){
  vec2 c=uR.xy+uR.zw*0.5, q=abs(vP-c)-uR.zw*0.5+vec2(uRad);
  float d=length(max(q,vec2(0.0)))+min(max(q.x,q.y),0.0)-uRad;
  vec4 fill=mix(uTop,uBot,clamp((vP.y-uR.y)/uR.w,0.0,1.0)); fill.a*=clamp(0.5-d,0.0,1.0);
  vec4 ring=uEdge; ring.a*=uEW>0.0?clamp(uEW*0.5+0.5-abs(d+uEW*0.5),0.0,1.0):0.0;
  vec4 glow=uGlow; glow.a*=uGR>0.0?exp(-max(d,0.0)/uGR)*clamp(d+0.5,0.0,1.0):0.0;
  oC=over(ring,over(fill,glow)); })";

// The BEYOND wordmark, built from thin luminous strokes (coordinates: letter height = 1).
static const char* FS_LOGO = R"(#version 330 core
in vec2 vP; out vec4 oC; uniform vec4 uR; uniform float uT, uA;
float seg(vec2 p,vec2 a,vec2 b){ vec2 ba=b-a; return length(p-a-ba*clamp(dot(p-a,ba)/dot(ba,ba),0.0,1.0)); }
float bowl(vec2 p,vec2 c,float r){ return p.x>=c.x?abs(length(p-c)-r):length(vec2(p.x-c.x,abs(p.y-c.y)-r)); }
float box(vec2 p,vec2 c,vec2 b,float r){ vec2 q=abs(p-c)-b+vec2(r); return abs(length(max(q,vec2(0.0)))+min(max(q.x,q.y),0.0)-r); }
float glyphs(vec2 p){
  float d=seg(p,vec2(0.0,0.0),vec2(0.0,1.0));
  d=min(d,seg(p,vec2(0.0,1.0),vec2(0.38,1.0))); d=min(d,seg(p,vec2(0.0,0.52),vec2(0.42,0.52))); d=min(d,seg(p,vec2(0.0,0.0),vec2(0.42,0.0)));
  d=min(d,bowl(p,vec2(0.38,0.76),0.24)); d=min(d,bowl(p,vec2(0.42,0.26),0.26));
  vec2 e=p-vec2(1.14,0.0); d=min(d,seg(e,vec2(0.0,0.0),vec2(0.0,1.0))); d=min(d,seg(e,vec2(0.0,1.0),vec2(0.62,1.0))); d=min(d,seg(e,vec2(0.0,0.52),vec2(0.52,0.52))); d=min(d,seg(e,vec2(0.0,0.0),vec2(0.62,0.0)));
  vec2 y=p-vec2(2.20,0.0); d=min(d,seg(y,vec2(0.0,1.0),vec2(0.36,0.48))); d=min(d,seg(y,vec2(0.72,1.0),vec2(0.36,0.48))); d=min(d,seg(y,vec2(0.36,0.48),vec2(0.36,0.0)));
  d=min(d,box(p,vec2(3.78,0.5),vec2(0.44,0.5),0.40));
  vec2 n=p-vec2(4.66,0.0); d=min(d,seg(n,vec2(0.0,0.0),vec2(0.0,1.0))); d=min(d,seg(n,vec2(0.0,1.0),vec2(0.68,0.0))); d=min(d,seg(n,vec2(0.68,0.0),vec2(0.68,1.0)));
  vec2 k=p-vec2(5.80,0.0); d=min(d,seg(k,vec2(0.0,0.0),vec2(0.0,1.0))); d=min(d,seg(k,vec2(0.0,1.0),vec2(0.22,1.0))); d=min(d,seg(k,vec2(0.0,0.0),vec2(0.22,0.0))); d=min(d,bowl(k,vec2(0.22,0.5),0.5));
  return d; }
void main(){
  float H=uR.w; vec2 p=vec2(vP.x-uR.x,uR.y+H-vP.y)/H; float d=glyphs(p), aa=1.2/H, th=0.036;
  float core=1.0-smoothstep(th-aa,th+aa,d), sw=(p.x-mod(uT*0.55,14.0)+3.0)*0.9, sweep=exp(-sw*sw);
  vec3 tint=mix(vec3(0.62,0.82,1.0),vec3(1.0,0.94,0.84),clamp(p.y,0.0,1.0));
  vec3 col=mix(tint,vec3(1.0),0.55+0.45*sweep)*core*(1.0+sweep*0.3);
  col+=vec3(0.28,0.56,1.0)*(exp(-d*22.0)*0.55+exp(-d*5.5)*0.14)+vec3(1.0,0.72,0.42)*exp(-d*14.0)*sweep*0.6;
  oC=vec4(col*uA,1.0); })";
