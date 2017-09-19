import scipy.sparse.linalg as LA
from scipy import sparse

import py_rootbox as rb    
from rb_tools import *

import xylem_flux 

# Simulate a root system
name = "Sorghum_bicolor_NA_NA"
dt = 30 # days
rs = rb.RootSystem()
rs.openFile(name)
rs.initialize() 
rs.simulate(dt) 

# Create graph
nodes = vv2a(rs.getNodes())/100 # convert from cm to m 
seg = seg2a(rs.getSegments())

# Adjacency matrix
A = sparse.coo_matrix((np.ones(seg.shape[0]),(seg[:,0],seg[:,1]))) 

# Parameters for flux model
rs_Kr = np.array([ 2.e-10, 2.e-10, 2.e-10, 2.e-10, 2.e-10, 2.e-11, 2.e-11 ]) # s/m; root hydraulic radial conductivity per root type 
rs_Kz = np.array([ 5.e-14, 5.e-14, 5.e-14, 5.e-14, 5e-14, 5e-14, 5e-14 ]) # m2*s; root hydraulic axial conductivity per root type 

soil_psi = -700 # static soil pressure J kg^-1

rho = 1e3 # kg / m^3      
g = 1.e-3*9.8065 # m / s^2   

pot_trans = np.array([-1.15741e-10]) # # m^3 s^-1 potential transpiration

# Conversions
rs_ana = rb.SegmentAnalyser(rs) 
radius = v2a(rs_ana.getScalar(rb.ScalarType.radius))/100. # convert from cm to m 
type = v2a(rs_ana.getScalar(rb.ScalarType.type))
kr = np.array(list(map(lambda t: rs_Kr[int(t)-1], type))) # convert from 'per type' to 'per segment'
kz = np.array(list(map(lambda t: rs_Kz[int(t)-1], type)))     
          
# Call back function for soil potential
soil = lambda x,y,z : soil_psi

# Calculate fluxes within the root system
Q, b = xylem_flux.linear_system(seg, nodes, radius, kr, kz, rho, g, soil) 
Q, b = xylem_flux.bc_neumann(Q, b, np.array([0]), np.array([pot_trans]))
x = LA.spsolve(Q, b) # direct
          
# Save results into vtp 
segP = nodes2seg(nodes,seg,x)# save vtp 
axial_flux = xylem_flux.axial_flux(x, seg, nodes, kz, rho, g)
radial_flux = xylem_flux.radial_flux(x, seg, nodes, radius, kr, soil)
net_flux = axial_flux+radial_flux
rs_ana.addUserData(a2v(segP),"pressure")
rs_ana.addUserData(a2v(axial_flux),"axial_flux")
rs_ana.addUserData(a2v(radial_flux),"radial_flux")
rs_ana.addUserData(a2v(net_flux),"net_flux")
rs_ana.write("results/example_5a.vtp")         
