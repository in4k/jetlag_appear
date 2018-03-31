/* File generated with Shader Minifier 1.1.4
 * http://www.ctrl-alt-test.fr
 */
#ifndef MAIN_SHADER_H_
# define MAIN_SHADER_H_

const char *main_shader_glsl =
 "#version 130\n"
 "uniform int F;"
 "uniform sampler2D S;"
 "float v=F/352800.;"
 "vec3 i=vec3(0.,.01,1.);"
 "vec4 f(vec2 v)"
 "{"
   "return texture2D(S,(v+.5)/textureSize(S,0));"
 "}"
 "float y=6.36e+06,z=6.38e+06;"
 "int x=128,m=16;"
 "float s=.76,e=s*s,t=8000.,l=1200.,r=10.;"
 "vec3 w=vec3(0.,-y,0.),n=vec3(5.8e-06,1.35e-05,3.31e-05),p=vec3(2.1e-05);"
 "vec4 c,d;"
 "vec3 u;"
 "float g,o;"
 "float h(vec3 v)"
 "{"
   "float i=v.y-400.,z=length(v.xz);"
   "v/=100.;"
   "vec2 y=floor(v.xz),W=y;"
   "float b=10.,m=10.;"
   "for(float C=-1.;C<=1.;C+=1.)"
     "for(float r=-1.;r<=1.;r+=1.)"
       "{"
         "vec2 x=y+vec2(C,r),q=x+.5,Y=floor(x/10.);"
         "d=f(Y);"
         "float e=dot(x,x);"
         "c=f(x);"
         "q+=(c.yz-.5)*smoothstep(100.,200.,e)*d.x;"
         "float s=abs(q.x-v.x)+abs(q.y-v.z);"
         "if(s<b)"
           "m=b,b=s,W=x;"
         "else"
           " if(s<m)"
             "m=s;"
       "}"
   "float x=m-b,Y=dot(W,W);"
   "c=f(W);"
   "d=f(W/10.);"
   "u=v-vec3(W.x,0.,W.y);"
   "o=step(.8,d.w);"
   "float s=smoothstep(10000.,1000.,z)*(1.-o),C=s*(1.+4.*smoothstep(.8,1.,d.x));"
   "g=(.3-x)*.5;"
   "i=min(v.y,max(v.y-c.x*C,g));"
   "return 100.*max(i,length(v)-100.);"
 "}"
 "vec3 a(vec3 v)"
 "{"
   "return normalize(vec3(h(v+i.yxx),h(v+i.xyx),h(v+i.xxy))-h(v));"
 "}"
 "float a(vec3 v,vec3 i,float f,float x)"
 "{"
   "float y=10.,m=f;"
   "for(int r=0;r<199;++r)"
     "{"
       "float s=h(v+i*f);"
       "f+=s*.37;"
       "if(s<.001*f||f>x)"
         "return f;"
       "if(s<y)"
         "y=s,m=f;"
     "}"
   "return m;"
 "}"
 "vec3 C,b,L,q;"
 "float Z=.5,Y=10.;"
 "float X(vec3 v)"
 "{"
   "return mix(max(0.,dot(L,v))/3.,max(0.,pow(dot(L,normalize(v-b)),Y)*(Y+8.)/24.),Z);"
 "}"
 "vec3 W=vec3(1.,.001,1.);"
 "float V(in vec3 v)"
 "{"
   "vec3 s=floor(v),W=fract(v);"
   "vec2 i=s.xy+vec2(37.,17.)*s.z+W.xy,m=textureLod(S,(i+.5)/textureSize(S,0),0.).yx;"
   "return mix(m.x,m.y,W.z);"
 "}"
 "float U(in vec3 v)"
 "{"
   "return.55*V(v)+.225*V(v*2.)+.125*V(v*3.99)+.0625*V(v*8.9);"
 "}"
 "float T(vec3 v)"
 "{"
   "float f=U(v*.0004);"
   "f=smoothstep(.44,.64,f);"
   "f*=f*40.;"
   "return f;"
 "}"
 "vec2 R(vec3 x)"
 "{"
   "float i=max(0.,length(x-w)-y);"
   "vec2 m=vec2(exp(-i/t),exp(-i/l));"
   "float s=5000.;"
   "if(s<i&&i<10000.)"
     "m.y+=step(length(x),50000.)*T(x+vec3(23175.7,0.,v*30.))*max(0.,sin(3.1415*(i-s)/s));"
   "return m;"
 "}"
 "float R(in vec3 v,in vec3 x,in float s)"
 "{"
   "vec3 m=v-w;"
   "float f=dot(m,x),W=dot(m,m)-s*s,z=f*f-W;"
   "if(z<0.)"
     "return-1.;"
   "float i=sqrt(z),y=-f-i,b=-f+i;"
   "return y>=0.?y:b;"
 "}"
 "vec3 T(vec3 v,vec3 i,float x)"
 "{"
   "float s=x/float(m);"
   "vec2 f=vec2(0.);"
   "for(int r=0;r<m;++r)"
     "{"
       "vec3 y=v+i*float(r)*s;"
       "f+=R(y)*s;"
     "}"
   "return exp(-(n*f.x+p*f.y));"
 "}"
 "vec3 R(vec3 v,vec3 i,float f,vec3 y)"
 "{"
   "vec2 C=vec2(0.,0.);"
   "vec3 q=vec3(0.),b=vec3(0.);"
   "float c=f/float(x);"
   "for(int Y=0;Y<x;++Y)"
     "{"
       "vec3 g=v+i*float(Y)*c;"
       "vec2 d=R(g)*c;"
       "C+=d;"
       "float t=R(g,W,z);"
       "if(t>0.)"
         "{"
           "float V=t/float(m);"
           "vec2 u=vec2(0.);"
           "for(int w=0;w<m;++w)"
             "{"
               "vec3 Z=g+W*float(w)*V;"
               "u+=R(Z)*V;"
             "}"
           "vec2 Z=u+C;"
           "vec3 w=exp(-(n*Z.x+p*Z.y));"
           "q+=w*d.x;"
           "b+=w*d.y;"
         "}"
       "else"
         " return vec3(0.);"
     "}"
   "float V=dot(i,W),Y=1.+V*V;"
   "vec2 d=vec2(.0596831*Y,.119366*(1.-e)*Y/((2.+e)*pow(1.+e-2.*s*V,1.5)));"
   "return y*exp(-(n*C.x+p*C.y))+r*(q*n*d.x+b*p*d.y);"
 "}"
 "float Q(float v)"
 "{"
   "return clamp(v,0.,1.);"
 "}"
 "void main()"
 "{"
   "vec2 s=vec2(1280.,720.),m=gl_FragCoord.xy/s*2.-1.;"
   "m.x*=s.x/s.y;"
   "v+=f(gl_FragCoord.xy+v*100.*vec2(17.,39.)).x;"
   "vec3 y=vec3(0.);"
   "C=vec3(mod(v*2.,10000.)-5000.,1000.+500.*sin(v/60.),mod(v*10.,10000.)-5000.);"
   "if(v<64.)"
     "r=10.*v/64.,y=vec3(10000.,15000.,-10000.);"
   "else"
     " if(v<128.)"
       "{"
         "C.y=1000.;"
         "float x=(v-64.)/64.;"
         "C=vec3(x*1000.,1500.,3000.);"
         "y=C+vec3(-300.,0.,-300.);"
         "y.y=10.;"
         "W.y+=.01*x*x*x;"
       "}"
     "else"
       " if(v<144.)"
         "W.y=.01;"
       "else"
         "{"
           "float x=(v-144.)/64.;"
           "C=vec3(mod(x*2.,10000.)-500.,1000.-200.*x,mod(x*10.,10000.)-5000.);"
           "y.y=4000.*x;"
           "W.y=.01+.5*x;"
           "r=10.+200.*max(0.,(v-196.)/16.);"
         "}"
   "W=normalize(W);"
   "b=normalize(C-y);"
   "vec3 x=normalize(cross(i.xzx,b));"
   "b=mat3(x,normalize(cross(b,x)),b)*normalize(vec3(m,-1.));"
   "vec3 e=vec3(0.);"
   "float w=50000.,d=a(C,b,0.,w);"
   "vec3 V=vec3(0.);"
   "if(d<w)"
     "{"
       "q=C+b*d;"
       "L=a(q);"
       "vec3 n=vec3(.3+.2*c.w)+.03*c.xyz;"
       "Y=200.;"
       "if(o>0.)"
         "{"
           "float t=step(.1,g);"
           "n=.05*mix(vec3(.4,.9,.3)-vec3(.2*f(q.xz).x),.8*vec3(.6,.8,.2),t);"
           "Z=.5-.5*(1.-t);"
           "Y=10.*(1.-t);"
         "}"
       "else"
         " if(q.y<1.)"
           "n=vec3(.01+.04*step(g,.07))+vec3(.7)*step(.145,g),Y=1.;"
       "e+=n*T(q,W,R(q,W,z))*r*X(W);"
       "vec3 t=vec3(-W.x,W.y,-W.z);"
       "e+=n*R(q,t,R(q,t,z),vec3(0.))*X(t);"
       "e+=n*vec3(.07)*X(normalize(vec3(1.)));"
       "e+=V;"
     "}"
   "else"
     " d=R(C,b,z);"
   "e=R(C,b,d,e);"
   "gl_FragColor=vec4(pow(e,vec3(1./2.2)),.5);"
 "}";

#endif // MAIN_SHADER_H_
