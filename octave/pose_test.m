#! /usr/bin/octave -q
# pose test vector generation

function pose_test
  # reflector model points, in millimeters in the common,
  # camera-centric coordinate system
  reflector_model = [0,0,0; -35,-50,-92.5;35,-50,-92.5]';
  # user head center with respect to the reflector model 
  # point zero (in mm)
  head_center = [0,-100,90];
  # focal  length, in pixels
  flp=863.0;

  angbase = 30;
  # centered and +/- 5 cm 
  tx_range = [0.0]; 
  #tx_range = [0.0,-50.0,+50.0]; 

  # centered and +/- 5 cm 
  ty_range = [0.0]; 
  #ty_range = [0.0,-50.0,+50.0]; # centered and +/- 5 cm 

  # 45 cm away and 45 +/- 5 cm 
  tz_range = [10000.0]; 
  #tz_range = [450.0,450.0-50.0,450.0+50.0]; 

  # centered and +/- 20 degrees 
  ax_range = [0.0];
  #ax_range = [0.0,-angbase*pi/180.0,+angbase*pi/180.0]; 

  # centered and +/- 20 degrees 
  #ay_range = [0.0];
  ay_range = [0.0,-angbase*pi/180.0,+angbase*pi/180.0]; 

  # centered and +/- 20 degrees 
  #az_range = [0.0];
  az_range = [0.0,-angbase*pi/180.0,+angbase*pi/180.0]; 

  ref_img = gen_image(reflector_model,flp,
                      tx_range(1),
                      ty_range(1),
                      tz_range(1),
                      ax_range(1),
                      ay_range(1),
                      az_range(1))
  ref_pos = position_reflector_model(reflector_model,flp,
                      tx_range(1),
                      ty_range(1),
                      tz_range(1),
                      ax_range(1),
                      ay_range(1),
                      az_range(1))
  ref_frame = alter92(reflector_model,flp,ref_img)
  [ref_tr, ref_rot] = compute_basis(ref_frame)
  for tx = tx_range
    for ty = ty_range
      for tz = tz_range
        for ax = ax_range
          for ay = ay_range
            for az = az_range
              new_pos = position_reflector_model(reflector_model,flp,tx,ty,tz,ax,ay,az);
              img = gen_image(reflector_model,flp,tx,ty,tz,ax,ay,az)
              nth_frame=octave_alter92_result=alter92(reflector_model,flp,img)
              [nth_tr, nth_rot] = compute_basis(nth_frame)
              result=(nth_rot*ref_rot')
              [eax, eay, eaz] = matrix2euler(result)
              actual=[ax,ay,az]/pi*180
            endfor
          endfor
        endfor
      endfor
    endfor
  endfor
  #total_err_mat
  #plot(total_err_mat);
endfunction

# returns a matrix of 2d(x y) column vector points of the 
# positions on the screen, in pixels (the screen center is
# at (0,0)
# the reflector model input must be in homogeneous coords!
function image = gen_image(reflector_model,flp,tx,ty,tz,ax,ay,az)
  # get the new model position
  repositioned_model = position_reflector_model(reflector_model,flp,tx,ty,tz,ax,ay,az);
  # extend to homogeneous coordinates
  working_reflector_model = r3_to_r4(repositioned_model);
  screen_coord_xform = [1,0,0,0; 0,1,0,0; 0,0,1,0;0,0,1/flp,0];
  screen_matrix =screen_coord_xform*working_reflector_model;
  # take back out of homogeneous coords
  image = r4_to_r3(screen_matrix);
  # strip off the z
  image = image(1:2,:);
endfunction

# returns a matrix of 3d(x y z) column vector points of the 
# rotated and translated model
function repositioned_model = position_reflector_model(reflector_model,flp,tx,ty,tz,ax,ay,az)
  # extend to homogeneous coordinates
  working_reflector_model = r3_to_r4(reflector_model);
  screen_coord_xform = [1,0,0,0; 0,1,0,0; 0,0,1,0;0,0,1/flp,0];
  translation_matrix = [1,0,0,0;0,1,0,0;0,0,1,0;tx,ty,tz,1]';
  rotation_matrix_3d = euler2matrix(ax,ay,az)
  rotation_matrix = [[rotation_matrix_3d;[0,0,0]], [0;0;0;1]]
  motion_xform_matrix = translation_matrix*rotation_matrix
  repositioned_model = motion_xform_matrix*working_reflector_model
  # take back out of homogeneous coords
  repositioned_model = r4_to_r3(repositioned_model);
endfunction

function [matrix] = euler2matrix(ax, ay, az)
  xrotation_matrix= [1,0,0; 0,cos(ax),-sin(ax); 0,sin(ax),cos(ax)];
  yrotation_matrix= [cos(ay),0,sin(ay); 0,1,0; -sin(ay),0,cos(ay)];
  zrotation_matrix= [cos(az),-sin(az),0; sin(az),cos(az),0;0,0,1];
  matrix = zrotation_matrix*yrotation_matrix*xrotation_matrix;
endfunction

function [ax,ay,az] = matrix2euler(R)
  ax=atan2(R(3,2),R(3,3))*180/pi;
  ay=-asin(R(3,1))*180/pi;
  az=atan2(R(2,1),R(1,1))*180/pi;
endfunction

# convert a 3d point columnvector matrix into 
# homogeneous coords (4d)
function r4m = r3_to_r4(m)
  r4m = [m;ones(1,columns(m))];
endfunction

# convert a 4d homogeneous coords (4d) columnvector 
# matrix into 3d points (columnvector matrix).
function r3m = r4_to_r3(m)
  r3m = m(1:3,:)./repmat(m(4,:),3,1);
endfunction

# convert an set of 2d columnvector points to
# a string as a series of comma seperated values.
# ordering: x0,y0,x1,y1,x2,y2,etc
function csvstr = gen_image_csv(img)
  csvstr = "";
  for colnum = 1:columns(img)
    for rownum = 1:rows(img)
      csvstr = strcat(csvstr,sprintf("%f,",img(rownum,colnum)));
    endfor
  endfor
  csvstr = strcat(csvstr,sprintf("0"));
endfunction

function res = normalize(v)
  res = v / norm(v);
endfunction

function res = scal(v1, v2)
  res = sum(v1' * v2);
endfunction

# given two vectors, generate an orthonormal basis matrix
# new basis has i,j,k in the column vectors
function [base] = get_base(v1, v2)
  #gram schmidt step 1
  v1 = normalize(v1);
  #gram schmidt step 2
  v2 = v2 - v1 * (scal(v1, v2));
  v2 = normalize(v2);
  # shortcut for the last vector
  v3 = cross(v1, v2);
  base = [v1'; v2'; v3'];
endfunction

# the alter92 algorithm for estimating a pose, with an
# added focal depth to attempt to compute z distance
function estimate = alter92(reflector_model,
                            flp,
                            image)
  m0 = reflector_model(:,1);
  m1 = reflector_model(:,2);
  m2 = reflector_model(:,3);
  pp0 = image(:,1);
  pp1 = image(:,2);
  pp2 = image(:,3);
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
  # FIXME: Add z-calculation
  ap0 = [pp0; 0] / s;
  ap1 = [pp1; 0+h1] / s;
  ap2 = [pp2; 0+h2] / s;
  estimate = [ap0,ap1,ap2];
endfunction

# given an alter92 estimate ,
# compute the relative translation and rotation
function [tr,rot] = compute_basis(ref)
  v1 = ref(:,2)-ref(:,1);
  v2 = ref(:,3)-ref(:,1);
  b = get_base(v1,v2);
  tr = ref(:,1);
  rot = b';
endfunction
