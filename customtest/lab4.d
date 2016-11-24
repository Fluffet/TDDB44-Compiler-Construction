program semtest1;
var
	t : array[10] of integer;
	a : integer;
	b : real;
	x : integer;
	y : integer;
	aby : real;

procedure func (a:integer);
var
	c : real;
begin
	c := a;
end;

begin
	a := 1;
	b := 2 + 1;
	a := t[a];
	func(1);
	x := a;
	if (a>3) then
		x := 20;
	end;
	a := 100;
	b := x;
end.



