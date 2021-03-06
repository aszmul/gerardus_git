/*Quadratic Curve Fitting Solution

  Find a plausable quadratic fit to a sample of points

  Nigel_Galloway@operamail.com
  February 1st., 2009
*/
set Sample;
param Sx {z in Sample};
param Sy {z in Sample};

var a;
var b;
var c;

equalz1 :sum{z in Sample} a*Sx[z]*Sx[z]*Sx[z]*Sx[z] + sum{z in Sample} b*Sx[z]*Sx[z]*Sx[z] + sum{z in Sample} c*Sx[z]*Sx[z] = sum{z in Sample} Sy[z]*Sx[z]*Sx[z];
equalz2 :sum{z in Sample} a*Sx[z]*Sx[z]*Sx[z] + sum{z in Sample} b*Sx[z]*Sx[z] + sum{z in Sample} c*Sx[z] = sum{z in Sample} Sy[z]*Sx[z];
equalz3 :sum{z in Sample} a*Sx[z]*Sx[z] + sum{z in Sample} b*Sx[z] + sum{z in Sample} c = sum{z in Sample} Sy[z];

solve;

printf "\nbest quadratic fit is:\n\ty = %f %s %fx %s %fx^2\n\n", c, if b < 0 then "-" else "+", abs(b), if a < 0 then "-" else "+", abs(a);

data;

param:
Sample:   Sx    Sy :=
  1         0    1
  2       0.5  0.9
  3         1  0.7
  4       1.5  1.5
  5       1.9    2
  6       2.5  2.4
  7         3  3.2
  8       3.5    2
  9         4  2.7
 10       4.5  3.5
 11         5    1
 12       5.5    4
 13         6  3.6
 14       6.6  2.7
 15         7  5.7
 16       7.6  4.6
 17       8.5    6
 18         9  6.8
 19        10  7.3
;

end;
