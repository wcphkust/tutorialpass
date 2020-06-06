## Test case
`test/interval_analysis.c`

## Current Output

Line 2:
x.addr: [-inf,inf]; y: [0,0];
Line 4:
x.addr: [0,-1]; y: [3,2];
Line 6:
x.addr: [0,-1]; y: [-3,-4];
Line 9:
x.addr: [1,1]; y: [0,-1];
Line 11:
x.addr: [1,0]; y: [0,-1];
Line 14:
x.addr: [0,-1]; y: [0,-1];

## Expected Output

Line 2:
x.addr: [-inf,inf]; y: [0,0];
Line 4:
x.addr: [-inf,4]; y: [-inf,7];
Line 6:
x.addr: [5,inf]; y: [2,inf];
Line 9:
x.addr: [1,1]; y: [-inf,inf];
Line 11:
x.addr: [2,100]; y: [-inf,inf];
Line 14:
x.addr: [100,inf]; y: [100,inf];