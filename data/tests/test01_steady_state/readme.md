# TestSuite Info

The `.cmdline` files hold custom command line option for the respective projects.
Files with `.ref` extension are reference projects for use with DELPHIN 6.


## Tests in this directory

### steady_state

Simple test with constant boundary conditions and a little condensate happening 
inside the construction

### steady_state_discretized

Same test but with pre-generated discretization stored in project file.
Checks if command-line option `--no-disc` is applyied correctly.

### steady_state_equiGrid

Same test as **steady_state**. 
Checks if command-line option `--disc=equi:0.02` is applyied correctly.

### steady_state_varGrid

Same test as **steady_state**. 
Checks if command-line option `--disc=var:0.005:1.7` is applyied correctly.


