%Transposition of orthogonal matrix is equivalent to inversion.


function pose
  dist = 10
  [p0,p1,p2] = position_cap(0,0,0,0,0,1000);
  p = [p0 p1 p2]
  pp0 = project_point(dist, p0);
  pp1 = project_point(dist, p1);
  pp2 = project_point(dist, p2);
  pp = [pp0 pp1 pp2]
  [ap0, ap1, ap2] = alter(pp0, pp1, pp2, dist);
  ap = [ap0 ap1 ap2]
  ref_pt = recover_ref(ap0, ap1, ap2)
  ref_angles = recover_angles(ap0, ap1, ap2)
endfunction

function [ref_angles] = recover_angles(ap0, ap1, ap2)
  [m0, m1, m2, ref,base, ref_in_base] = get_cap;
  new_base = get_base(ap1 - ap0, ap2 - ap0);
  
  [pitch, yaw, roll] = matrix2euler(base' * new_base);
  ref_angles = [pitch/pi*180, yaw/pi*180, roll/pi*180];
endfunction

function [new_ref] = recover_ref(ap0, ap1, ap2)
  [m0, m1, m2, ref,base, ref_in_base] = get_cap;
  
  new_base = get_base(ap1 - ap0, ap2 - ap0);
%  new_ref = ap0 + ((new_base ^ -1) * ref_in_base);
  new_ref = ap0 + (new_base' * ref_in_base);
  norm(m0 - ref) - norm(ap0 - new_ref)
  norm(m1 - ref) - norm(ap1 - new_ref)
  norm(m2 - ref) - norm(ap2 - new_ref)
endfunction

function [ap0, ap1, ap2] = alter(pp0, pp1, pp2, dist)
  [m0, m1, m2, ref] = get_cap;
  R01 = norm(m0-m1);
  R02 = norm(m0-m2);
  R12 = norm(m1-m2);
  d01 = norm(pp0-pp1);
  d02 = norm(pp0-pp2);
  d12 = norm(pp1-pp2);
  a = (R01 + R02+ R12)*(-R01 + R02 + R12)*(R01 - R02 + R12)*(R01 + R02 - R12);
  b = d01^2*(-R01^2 + R02^2 + R12^2) + d02^2*(R01^2 - R02^2 + R12^2) + \
  	d12^2*(R01^2 + R02^2 - R12^2);
  c = (d01 + d02+ d12)*(-d01 + d02 + d12)*(d01 - d02 + d12)*(d01 + d02 - d12);
  
  s = sqrt((b + sqrt(b ^ 2 - a * c)) / a);
  
  if ((d01^2 + d02^2 - d12^2) <= (s^2 * (R01^2 + R02^2 - R12^2)))
    sigma = 1;
  else
    sigma = -1;
  endif  

  h1 = -sqrt((s * R01)^2 - d01^2);
  h2 = -sigma * sqrt((s * R02)^2 - d02^2);
  
  ap0 = [pp0; dist] / s;
  ap1 = [pp1; dist+h1] / s;
  ap2 = [pp2; dist+h2] / s;
endfunction


function res = scal(v1, v2)
  res = sum(v1' * v2);
endfunction

function res = normalize(v)
  res = v / norm(v);
endfunction

function [base] = get_base(v1, v2)
  v1 = normalize(v1);
  
  v2 = v2 - v1 * (scal(v1, v2));
  v2 = normalize(v2);
  
  v3 = cross(v1, v2);
  
  base = [v1'; v2'; v3'];
endfunction

function [matrix] = euler2matrix(pitch, yaw, roll)
  mx = [	1		 0		  0	0;
  		0	cos(pitch)	-sin(pitch)	0;
        	0	sin(pitch)	 cos(pitch)	0;
        	0		 0		  0	1];
                
  my = [ cos(yaw)	 	 0	  -sin(yaw)	0;
  	 	0		 1		  0	0;
         sin(yaw)		 0	   cos(yaw)	0;
         	0		 0		  0	1];
  
  mz = [cos(roll)	-sin(roll)		  0	0;
  	sin(roll)	 cos(roll)		  0	0;
        	0		 0		  1	0;
  		0		 0		  0	1];
  matrix = mx * my * mz;
endfunction

function [pitch, yaw, roll] = matrix2euler(matrix)
  yaw = -asin(matrix(1,3));
  yc = cos(yaw);
  if (abs(yc) > 1e-5)
    pitch = atan2(-matrix(2,3)/yc, matrix(3,3)/yc);
    roll = atan2(-matrix(1,2)/yc, matrix(1,1)/yc);
  else
    pitch = 0;
    roll = atan2(matrix(2,1), matrix(2,2));
  endif
endfunction


%projects on plane at z=dist
function [p_pt] = project_point(dist, pt)
  k = dist/pt(3);
  p_pt = [k*pt(1); k*pt(2)];  %%PERSPECTIVE
endfunction


function [p0, p1, p2] = position_cap(pitch, yaw, roll, tran_x, tran_y, tran_z)
  [m0, m1, m2, ref] = get_cap();
  rp0 = m0 - ref;
  rp1 = m1 - ref;
  rp2 = m2 - ref;
  
  translation = zeros(4,4);
  translation(1:3, 4) = [tran_x; tran_y; tran_z];
  tm = euler2matrix(pitch/180*pi, yaw/180*pi, roll/180*pi) + translation;
  
  p0 = (tm * [rp0; 1])(1:3);
  p1 = (tm * [rp1; 1])(1:3);
  p2 = (tm * [rp2; 1])(1:3);
endfunction

function [m0, m1, m2, ref, base, ref_in_base] = get_cap
  x = 168; %between front pts 
  y = 90;  %elevation of the one above
  z = 100; %distance 
  
  ref_y = 150; %reference point...
  ref_z = 150;
  
  
  m0  = [    0;      y;      z]; % this point will be center of new coord system
  m1  = [ -x/2;      0;      0];
  m2  = [  x/2;      0;      0];
  ref = [    0; -ref_y;  ref_z];
  
  base = get_base(m1 - m0, m2 - m0)
  ref_in_base = base * (ref - m0)

%  test = [m0 m1 m2] ^ -1;
%  test * ref = ref_in_base;

  
%  m2_in_base = base * m2'
%  M = [base, -m2_in_base; zeros(1,3), 1]
%  M * [ref'; 1]
  
endfunction
