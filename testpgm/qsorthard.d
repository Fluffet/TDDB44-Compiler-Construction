program qsort;

const SIZE = 20;

var s : array[SIZE] of integer;

#include "stdio.d"

procedure readsequence;
var i : integer;
begin
/*
    s[0] := 1;
    s[1] := 7;
    s[2] := 9;
    s[3] := 4;
    s[4] := 96;
    s[5] := 24;
    s[6] := 97;
    s[7] := 123;
    s[8] := 645645;
    s[9] := 93;
    s[10] := 5;
    s[11] := 3;
    s[12] := 1002;
    s[13] := 584;
    s[14] := 27;
    s[15] := 64;
    s[16] := 773;
    s[17] := 634;
    s[18] := 673;
    s[19] := 20;
*/
    i := 0;
    while (i < SIZE) do
	s[i] := read_int();
	i := i + 1;
    end;
end;

procedure writesequence;
const SPACE = 32;
var i : integer;
begin
    i := 0;
    while (i < SIZE) do
	write_int(s[i]);
	write(SPACE);
	i := i + 1;
    end;
end;

procedure quicksort(l : integer; r : integer);
var
    i : integer;
    j : integer;
    t : integer;
    pivot : integer;
begin
    i := l; 
    j := r;
    pivot := s[(l+r) div 2];
    while not(i > j) do
        while (s[i] < pivot) do
	    i := i + 1;
	end;
        while (pivot < s[j]) do
	    j := j - 1;
	end;
        if (i - j < 1) then
            t := s[i];
	    s[i] := s[j];
	    s[j] := t;
	    i := i + 1;
	    j := j - 1;
	end;
    end;
    if (j > l) then
	quicksort(l, j);
    end;
    if (r > i) then
	quicksort(i, r);
    end;
end;

begin
    readsequence();
    quicksort(0, SIZE-1);
    writesequence();
end.
