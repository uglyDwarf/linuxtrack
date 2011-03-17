%here is my idea on how to create the sensitivity curves...
%I think we should be able to get such a form, that given x, we'll be able to
%get y (no creation of big tables...)
function bezier
figure;
hold on;

for i =6
  bezier_plot(i/10);
  bezier1_plot(i/10);
  
endfor
hold off;
endfunction

function bezier_plot(pt)

p0=[0;0];
p1=[pt; 1-pt];
p2=[1;1];

for T = 1:100
  t=T/100;
  B = [(1-t)^2; 2*t *(1-t); t^2];
  b(T,:) = [B(1)*p0(1) + B(2)*p1(1) + B(3)*p2(1); B(1)*p0(2) + B(2)*p1(2) + B(3)*p2(2)];
endfor

plot(b(:,1), b(:,2), 'r');

endfunction


function bezier1_plot(pt)

p0=[0;0];
p1=[pt; 1-pt];
p2=[1;1];

a = p0(1) - 2 * p1(1) + p2(1);
b = 2 * (p1(1) - p0(1));

for X = 1:20
  x=X/20;
  c = p0(1) - x;
  
  t = (-b + sqrt(b^2 - 4*a*c)) / (2 * a);
  
  B = [(1-t)^2; 2*t *(1-t); t^2];
  b1(X,:) = [x; B(1)*p0(2) + B(2)*p1(2) + B(3)*p2(2)];
endfor
b1
plot(b1(:,1), b1(:,2), 'g.+');

endfunction


