%here is my idea on how to create the sensitivity curves...
%I think we should be able to get such a form, that given x, we'll be able to
%get y (no creation of big tables...)
function bezier
figure(1);
hold on;

for i = 0:10
  bezier_plot(i/10);
endfor
hold off;
endfunction

function bezier_plot(pt)

t=(1:100)/100;
p0=[0;0];
p1=[pt; 1-pt];
p2=[1;1];

for T = 1:100
  t=T/100;
  B = [(1-t)^2; 2*t *(1-t); t^2];
  b(T,:) = [B(1)*p0(1) + B(2)*p1(1) + B(3)*p2(1); B(1)*p0(2) + B(2)*p1(2) + B(3)*p2(2)];
endfor

plot(b(:,1), b(:,2));

endfunction

