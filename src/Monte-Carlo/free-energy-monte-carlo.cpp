#include <stdio.h>
#include <time.h>
#include <cassert>
#include <math.h>
#include <stdlib.h>
#include <popt.h>
#include <sys/stat.h>
#include "handymath.h"
#include "vector3d.h"
#include "Monte-Carlo/square-well.h"
#include "Monte-Carlo/InitBox.h"
#include "version-identifier.h"

// ------------------------------------------------------------------------------
// Notes on conventions and definitions used
// ------------------------------------------------------------------------------
//
// All coordinates are cartesion.
//
// The coordinates x, y, z are always floating point numbers that refer to real
// locations within the cell.
//
// The coordinates x_i, y_i, z_i are always integers that refer to the bin number
// of the respective coordinate such that x_i refers to a bin of thickness dx
// centered at x.
//
// The symbols e, e_i, and de are used as general coordinates.
//
// Def: If two objects, a and b, are closer than a.R + b.R + neighbor_R + dn,
// then they are neighbors.
//
// Neighbors are used to drastically reduce the number of collision tests needed.
//
// Def: The neighborsphere of an object, a, is the sphere within which
// everything is a neighbor of a.
// Note that this sphere has a well defined center, but it does not have
// a well defined radius unless all obects are circumscribed by spheres of
// the same radius, but this does not affect anything.


// ------------------------------------------------------------------------------
// Global Constants
// ------------------------------------------------------------------------------

const int x = 0;
const int y = 1;
const int z = 2;

// ------------------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------------------

// Tests validity of a shrunken version of the cell
static bool overlap_in_small_cell(sw_simulation &sw, double scaling_factor);

// States how long it's been since last took call.
static void took(const char *name);

// Saves the locations of all balls to a file.
inline void save_locations(const ball *p, int N, const char *fname,
                           const double len[3], const char *comment="");

// The following functions only do anything if debug is true:

// Prints the location and radius of every ball
// As well as whether any overlap or are outside the cell.
inline void print_all(const ball *p, int N, double len[3]);

// Same as print_all, but only prints information for one ball,
// and does not do overlap tests.
inline void print_one(const ball &a, int id, const ball *p, int N,
                      double len[3], int walls);

// Only print those balls that overlap or are outside the cell
// Also prints those they overlap with
inline void print_bad(const ball *p, int N, double len[3], int walls);

// Checks to make sure that every ball is his neighbor's neighbor.
inline void check_neighbor_symmetry(const ball *p, int N);

// Check whether it is a reasonable time to save data according to our
// heuristic.
bool time_to_save();

//packs a cube with atoms using the rain method
bool pack_cube(sw_simulation &sw);

//loads baseAtoms.txt
INITBOX *load_cube(char *data_dir);

int main(int argc, const char *argv[]) {
  printf("Running %s version %s\n", argv[0], version_identifier());
  for (int i=0; argv[i]; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");
  took("Starting program");
  // ----------------------------------------------------------------------------
  // Define "Constants" -- set from arguments then unchanged
  // ----------------------------------------------------------------------------

  // NOTE: debug can slow things down VERY much
  int debug = false;

  sw_simulation sw;

  sw.sticky_wall = 0;
  sw.len[0] = sw.len[1] = sw.len[2] = 1;
  sw.walls = 0;
  sw.N = 10;
  sw.translation_scale = 0.05;

  unsigned long int seed = 0;

  char *data_dir = new char[1024];
  sprintf(data_dir, "free-energy-data");
  char *filename = new char[1024];
  sprintf(filename, "default_filename");
  char *filename_suffix = new char[1024];
  sprintf(filename_suffix, "default_filename_suffix");
  long simulation_runs = 1000000;
  double acceptance_goal = .4;
  double R = 1;
  const double well_width = 1;
  double ff = 0.3;
  double ff_small = -1;
  bool force_cube=false;
  bool pack_and_save_cube=false;
  double neighbor_scale = 2;
  double de_g = 0.05;
  double max_rdf_radius = 10;
  int totime = 0;
  double scaling_factor = 1.0;
  int small_cell_check_period = (1 * sw.N * sw.N)/10;

  poptContext optCon;
  // ----------------------------------------------------------------------------
  // Set values from parameters
  // ----------------------------------------------------------------------------
  poptOption optionsTable[] = {
    {"N", '\0', POPT_ARG_INT, &sw.N, 0, "Number of balls to simulate", "INT"},
    {"ff", '\0', POPT_ARG_DOUBLE, &ff, 0, "Filling fraction. If specified, the "
     "cell dimensions are adjusted accordingly without changing the shape of "
     "the cell."},
    {"ff_small", '\0', POPT_ARG_DOUBLE, &ff_small, 0, "Small filling fraction. If specified, "
     "This sets the desired filling fraction of the shrunk cell. Otherwise it defaults to ff."},
    {"force_cube", '\0', POPT_ARG_NONE, &force_cube, 0, "forces box to be a cube", "BOOLEAN"},
    {"pack_and_save_cube", '\0', POPT_ARG_NONE, &pack_and_save_cube, 0, "packs cube, save positions, then returns", "BOOLEAN"},
    {"walls", '\0', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &sw.walls, 0,
     "Number of walled dimensions (dimension order: x,y,z)", "INT"},
    {"runs", '\0', POPT_ARG_LONG | POPT_ARGFLAG_SHOW_DEFAULT, &simulation_runs,
     0, "Number of \"runs\" for which to run the simulation", "INT"},
    {"de_g", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT, &de_g, 0,
     "Resolution of distribution functions", "DOUBLE"},
    {"max_rdf_radius", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT,
     &max_rdf_radius, 0, "Set maximum radius for RDF data collection", "DOUBLE"},
    {"lenx", '\0', POPT_ARG_DOUBLE, &sw.len[x], 0,
     "Relative cell size in x dimension", "DOUBLE"},
    {"leny", '\0', POPT_ARG_DOUBLE, &sw.len[y], 0,
     "Relative cell size in y dimension", "DOUBLE"},
    {"lenz", '\0', POPT_ARG_DOUBLE, &sw.len[z], 0,
     "Relative cell size in z dimension", "DOUBLE"},
    {"filename", '\0', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &filename, 0,
     "Base of output file names", "STRING"},
    {"filename_suffix", '\0', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
     &filename_suffix, 0, "Output file name suffix", "STRING"},
    {"data_dir", '\0', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &data_dir, 0,
     "Directory in which to save data", "data_dir"},
    {"neighbor_scale", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT,
     &neighbor_scale, 0, "Ratio of neighbor sphere radius to interaction scale "
     "times ball radius. Drastically reduces collision detections","DOUBLE"},
    {"translation_scale", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT,
     &sw.translation_scale, 0, "Standard deviation for translations of balls, "
     "relative to ball radius", "DOUBLE"},
    {"seed", '\0', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &seed, 0,
     "Seed for the random number generator", "INT"},
    {"acceptance_goal", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT,
     &acceptance_goal, 0, "Goal to set the acceptance rate", "DOUBLE"},
    {"time", '\0', POPT_ARG_INT, &totime, 0,
     "Timing of display information (seconds)", "INT"},
    {"R", '\0', POPT_ARG_DOUBLE | POPT_ARGFLAG_SHOW_DEFAULT,
     &R, 0, "Ball radius (for testing purposes; should always be 1)", "DOUBLE"},
    {"debug", '\0', POPT_ARG_NONE, &debug, 0, "Debug mode", "BOOLEAN"},
    {"sf", '\0', POPT_ARG_DOUBLE, &scaling_factor, 0,
      "Factor by which to scale the small cell", "DOUBLE"},
    {"sc_period", '\0', POPT_ARG_INT, &small_cell_check_period, 0,
     "Check the small cell every P iterations", "INT"},
    POPT_AUTOHELP
    POPT_TABLEEND
  };
  optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "[OPTION...]\nNumber of balls and filling "
                         "fraction or cell dimensions are required arguments.");

  int c = 0;
  // go through arguments, set them based on optionsTable
  while((c = poptGetNextOpt(optCon)) >= 0);
  if (c < -1) {
    fprintf(stderr, "\n%s: %s\n", poptBadOption(optCon, 0), poptStrerror(c));
    return 1;
  }
  poptFreeContext(optCon);

  // ----------------------------------------------------------------------------
  // Verify we have reasonable arguments and set secondary parameters
  // ----------------------------------------------------------------------------

  if(sw.walls >= 2){
    printf("Code cannot currently handle walls in more than one dimension.\n");
    return 254;
  }
  if(sw.walls > 3){
    printf("You cannot have walls in more than three dimensions.\n");
    return 254;
  }
  if(well_width < 1){
    printf("Interaction scale should be greater than (or equal to) 1.\n");
    return 254;
  }

  if(ff_small != -1 && scaling_factor != 1.0){
    printf("You can't specify both the small filling fraction and the scaling factor.");  
    return 1;
  }


  if (ff != 0) {
    // The user specified a filling fraction, so we must make it so!
    const double volume = 4*M_PI/3*R*R*R*sw.N/ff;
    const double min_cell_width = 2*sqrt(2)*R; // minimum cell width
    const int numcells = (sw.N+3)/4; // number of unit cells we need
    const int max_cubic_width
      = pow(volume/min_cell_width/min_cell_width/min_cell_width, 1.0/3);
    if (max_cubic_width*max_cubic_width*max_cubic_width >= numcells) {
      // We can get away with a cubic cell, so let's do so.  Cubic
      // cells are nice and comfortable!
      sw.len[x] = sw.len[y] = sw.len[z] = pow(volume, 1.0/3);
    } else {
      printf("N = %d\n", sw.N);
      printf("ff = %g\n", ff);
      printf("min_cell_width = %g\n", min_cell_width);
      printf("volume**1/3 = %g\n", pow(volume, 1.0/3));
      printf("max_cubic_width**3 = %d, numcells = %d\n",
             max_cubic_width*max_cubic_width*max_cubic_width, numcells);
      // A cubic cell won't work with our initialization routine, so
      // let's go with a lopsided cell that should give us something
      // that will work.
      int xcells = int( pow(numcells, 1.0/3) );
      int cellsleft = (numcells + xcells - 1)/xcells;
      int ycells = int( sqrt(cellsleft) );
      int zcells = (cellsleft + ycells - 1)/ycells;

      // The above should give a zcells that is largest, followed by
      // ycells and then xcells.  Thus we make the lenz just small
      // enough to fit these cells, and so on, to make the cell as
      // close to cubic as possible.
      sw.len[z] = zcells*min_cell_width;
      if (xcells == ycells) {
        sw.len[x] = sw.len[y] = sqrt(volume/sw.len[z]);
      } else {
        sw.len[y] = min_cell_width*ycells;
        sw.len[x] = volume/sw.len[y]/sw.len[z];
      }
      printf("Using lopsided %d x %d x %d cell (total goal %d)\n",
             xcells, ycells, zcells, numcells);
    }
  }

  // if the user didn't specifiy a small filling fraction, then we should get it.
  if(ff_small == -1){
      ff_small = (4*M_PI/3*R*R*R*sw.N)/
        (sw.len[x]*sw.len[y]*sw.len[z]*scaling_factor*scaling_factor*scaling_factor);
  }
  // if they did, then we need to get the scale factor
  else{
    scaling_factor = std::cbrt(ff/ff_small);
  }


  printf("\nSetting cell dimensions to (%g, %g, %g).\n",
         sw.len[x], sw.len[y], sw.len[z]);
  printf("\nFilling fraction of small cell is %g", ff_small);
  if (sw.N <= 0 || simulation_runs < 0 || R <= 0 ||
      neighbor_scale <= 0 || sw.translation_scale < 0 ||
      sw.len[x] < 0 || sw.len[y] < 0 || sw.len[z] < 0) {
    fprintf(stderr, "\nAll parameters must be positive.\n");
    return 1;
  }

  const double eta = (double)sw.N*4.0/3.0*M_PI*R*R*R/(sw.len[x]*sw.len[y]*sw.len[z]);
  if (eta > 1) {
    fprintf(stderr, "\nYou're trying to cram too many balls into the cell. "
            "They will never fit. Filling fraction: %g\n", eta);
    return 7;
  }

  // If a filename was not selected, make a default
  if (strcmp(filename, "default_filename") == 0) {
    char *wall_tag = new char[100];
    if(sw.walls == 0) sprintf(wall_tag,"periodic");
    else if(sw.walls == 1) sprintf(wall_tag,"wall");
    else if(sw.walls == 2) sprintf(wall_tag,"tube");
    else if(sw.walls == 3) sprintf(wall_tag,"box");
    sprintf(filename, "%s-ww%04.2f-ff%04.2f-N%i-sf%f",
            wall_tag, well_width, eta, sw.N, scaling_factor);
    printf("\nUsing default file name: ");
    delete[] wall_tag;
  }
  else
    printf("\nUsing given file name: ");
  // If a filename suffix was specified, add it
  if (strcmp(filename_suffix, "default_filename_suffix") != 0)
    sprintf(filename, "%s-%s", filename, filename_suffix);
  printf("%s\n",filename);

  printf("------------------------------------------------------------------\n");
  printf("Running %s with parameters:\n", argv[0]);
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') printf("\n");
    printf("%s ", argv[i]);
  }
  printf("\nUsing scaling factor of %f", scaling_factor);
  printf("\n");
  if (totime > 0) printf("Timing information will be displayed.\n");
  if (debug) printf("DEBUG MODE IS ENABLED!\n");
  else printf("Debug mode disabled\n");
  printf("------------------------------------------------------------------\n\n");

  // ----------------------------------------------------------------------------
  // Define sw_simulation variables
  // ----------------------------------------------------------------------------

  sw.iteration = 0; // start at zeroeth iteration
  sw.max_entropy_state = 0;
  sw.min_energy_state = 0;
  sw.energy = 0;
  sw.min_important_energy = 0;

  // translation distance should scale with ball radius
  sw.translation_scale *= R;

  // neighbor radius should scale with radius and interaction scale
  sw.neighbor_R = neighbor_scale*R*well_width;

  // Find the upper limit to the maximum number of neighbors a ball could have
  sw.max_neighbors = max_balls_within(2+neighbor_scale*well_width);

  // Energy histogram and weights
  sw.interaction_distance = 2*R*well_width;
  sw.energy_levels = 1; // hard spheres can only have one energy!
  sw.energy_histogram = new long[sw.energy_levels]();
  sw.ln_energy_weights = new double[sw.energy_levels]();

  // Observed and sampled energies
  sw.pessimistic_observation = new bool[sw.energy_levels]();
  sw.pessimistic_samples = new long[sw.energy_levels]();
  sw.optimistic_samples = new long[sw.energy_levels]();

  // Transitions from one energy to another
  sw.biggest_energy_transition = max_balls_within(sw.interaction_distance);
  sw.transitions_table =
    new long[sw.energy_levels*(2*sw.biggest_energy_transition+1)]();

  // Walker histograms
  sw.walkers_up = new long[sw.energy_levels]();


  printf("memory use estimate = %.2g G\n\n",
         8*double(6*sw.energy_levels)/1024/1024/1024);

  sw.balls = new ball[sw.N];

  if(totime < 0) totime = 10*sw.N;

  // Initialize the random number generator with our seed
  random::seed(seed);

  // ----------------------------------------------------------------------------
  // Set up the initial grid of balls
  // ----------------------------------------------------------------------------

  for(int i = 0; i < sw.N; i++) // initialize ball radii
    sw.balls[i].R = R;

    //###########################  pack and save  ####################
  if(pack_and_save_cube==true){
	  INITBOX *box=load_cube(data_dir);
	  if(box!=NULL && box->numAtoms<sw.N) {delete box; box=NULL;}
	  if(box!=NULL){//---------loaded baseAtoms up to max of sw.N
		  printf("loading from baseAtoms.txt\n");
		  for(int count=0;count<(100*box->numAtoms);count++){
			  //mixes indices so the culled atoms are random
			  //atoms are only culled from the end...
			  int n1=floor(random::ran()*(box->numAtoms));
			  int n2=floor(random::ran()*(box->numAtoms));
			  double xTemp=box->list[n1].x;
			  double yTemp=box->list[n1].y;
			  double zTemp=box->list[n1].z;
			  box->list[n1].x=box->list[n2].x;
			  box->list[n1].y=box->list[n2].y;
			  box->list[n1].z=box->list[n2].z;
			  box->list[n2].x=xTemp;
			  box->list[n2].y=yTemp;
			  box->list[n2].z=zTemp;
		  }
		  //box2 is used to cull atoms from the end (if needed)
		  //box2 is simulated to mix the atoms up after culling
		  INITBOX *box2=new INITBOX(box->lx);
		  for(int i=0;i<sw.N;i++) box2->addAtom(box->list[i].x,box->list[i].y,box->list[i].z);
		  delete box;
		  box2->temperature=1000.0;
		  for(int i=0;i<100;i++) box2->simulate(sw.N*100);//mix box
		  sw.len[x] = sw.len[y] = sw.len[z] = box2->lx;
		  for(int i=0;i<sw.N;i++) {
			  sw.balls[i].pos=vector3d(box2->list[i].x,box2->list[i].y,box2->list[i].z);
		  }
		  delete box2;
	  }
	  else{//if no baseAtoms.txt then try to pack the box
		if(ff==0){printf("must initialize using --ff\n"); return 254;}
		printf("building baseAtoms.txt\n");
		const double volume = 4*M_PI/3*R*R*R*sw.N/ff;
		sw.len[x] = sw.len[y] = sw.len[z] = pow(volume, 1.0/3);
		if(pack_cube(sw)==false) {printf("could not fill box\n"); return 254;}
	}//done initializing positions
	  mkdir(data_dir, 0777); // create save directory
	  
	  char *posData = new char[4096];
	  sprintf(posData,"%s/baseAtoms.txt",data_dir);
	  FILE *pos_out = fopen((const char *)posData, "w");
	  sprintf(posData,"L = %g\nN = %i\n",sw.len[x],sw.N);
	  fprintf(pos_out,"%s",posData);
	  for(int i=0;i<sw.N;i++){
		sprintf(posData,"%g %g %g",
				sw.balls[i].pos.x,sw.balls[i].pos.y,sw.balls[i].pos.z);
		fprintf(pos_out,"%s",posData);
		if(i+1<sw.N) fprintf(pos_out,"\n");
	  }
      fclose(pos_out);
      delete[] posData;
      took("Placement");
      return 0;
  }
  //###########################  DONE pack and save  #################
  
  if(force_cube==true){//try to load baseAtoms.txt
	  if(ff==0){printf("must initialize using --ff\n"); return 254;}
	  printf("attempting to load baseAtoms.txt\n");
	  const double volume = 4*M_PI/3*R*R*R*sw.N/ff;
	  sw.len[x] = sw.len[y] = sw.len[z] = pow(volume, 1.0/3);
	  INITBOX *box=load_cube(data_dir);
	  if(box==NULL) {
		  printf("requires initialization on the smallest box\nuse the --pack_and_save --ff -N   flags\n");
		  return 254; }
	  if(box->numAtoms<sw.N){
		  printf("boxAtoms.txt contains less atoms than expected\n");
		  delete box;
		  return 254; }
	  if(box->lx>sw.len[x]){
		  printf("baseAtoms must be in a box smaller than current simulation\n");
		  delete box;
		  return 254; }
	  box->temperature=1000.0;
	  for(int i=0;i<100;i++) box->simulate(sw.N*100);//mix box
      double scaleFactor=sw.len[x]/box->lx;
	  for(int i=0;i<sw.N;i++){
          double x=(box->list[i].x)*scaleFactor;
          double y=(box->list[i].y)*scaleFactor;
          double z=(box->list[i].z)*scaleFactor;
		  sw.balls[i].pos=vector3d(x,y,z);
	  }
	  delete box;
  }
  else {
	  // Balls will be initially placed on a face centered cubic (fcc) grid
	  // Note that the unit cells need not be actually "cubic", but the fcc grid will
      //   be stretched to cell dimensions
	  const double min_cell_width = 2*sqrt(2)*R; // minimum cell width
	  const int spots_per_cell = 4; // spots in each fcc periodic unit cell
	  int cells[3]; // array to contain number of cells in x, y, and z dimensions
	  for(int i = 0; i < 3; i++){
	    cells[i] = int(sw.len[i]/min_cell_width); // max number of cells that will fit
	  }
	
	  // It is usefull to know our cell dimensions
	  double cell_width[3];
	  for(int i = 0; i < 3; i++) cell_width[i] = sw.len[i]/cells[i];
	
	  // If we made our cells to small, return with error
	  for(int i = 0; i < 3; i++){
	    if(cell_width[i] < min_cell_width){
	      printf("Placement cell size too small: (%g,  %g,  %g) coming from (%g, %g, %g)\n",
	             cell_width[0],cell_width[1],cell_width[2],
	             sw.len[0], sw.len[1], sw.len[2]);
	      printf("Minimum allowed placement cell width: %g\n",min_cell_width);
	      printf("Total simulation cell dimensions: (%g,  %g,  %g)\n",
	             sw.len[0],sw.len[1],sw.len[2]);
	      printf("Fixing the chosen ball number, filling fractoin, and relative\n"
	             "  simulation cell dimensions simultaneously does not appear to be possible\n");
	      return 176;
	    }
	  }
	
	  // Define ball positions relative to cell position
	  vector3d* offset = new vector3d[4]();
	  offset[x] = vector3d(0,cell_width[y],cell_width[z])/2;
	  offset[y] = vector3d(cell_width[x],0,cell_width[z])/2;
	  offset[z] = vector3d(cell_width[x],cell_width[y],0)/2;
	
	  // Reserve some spots at random to be vacant
	  const int total_spots = spots_per_cell*cells[x]*cells[y]*cells[z];
	  bool *spot_reserved = new bool[total_spots]();
	  int p; // Index of reserved spot
	  for(int i = 0; i < total_spots-sw.N; i++) {
	    p = floor(random::ran()*total_spots); // Pick a random spot index
	    if(spot_reserved[p] == false) // If it's not already reserved, reserve it
	      spot_reserved[p] = true;
	    else // Otherwise redo this index (look for a new spot)
	      i--;
	  }
	
	  // Place all balls in remaining spots
	  int b = 0;
	  for(int i = 0; i < cells[x]; i++) {
	    for(int j = 0; j < cells[y]; j++) {
	      for(int k = 0; k < cells[z]; k++) {
	        for(int l = 0; l < 4; l++) {
	          if(!spot_reserved[i*(4*cells[z]*cells[y])+j*(4*cells[z])+k*4+l]) {
	            sw.balls[b].pos = vector3d(i*cell_width[x],j*cell_width[y],
	                                       k*cell_width[z]) + offset[l];
	            b++;
	          }
	        }
	      }
	    }
	  }
	  delete[] offset;
	  delete[] spot_reserved;
  }
  took("Placement");

  // ----------------------------------------------------------------------------
  // Print info about the initial configuration for troubleshooting
  // ----------------------------------------------------------------------------

  {
    int most_neighbors =
      initialize_neighbor_tables(sw.balls, sw.N, sw.neighbor_R,
                                 sw.max_neighbors, sw.len, sw.walls);
    if (most_neighbors < 0) {
      fprintf(stderr, "The guess of %i max neighbors was too low. Exiting.\n",
              sw.max_neighbors);
      return 1;
    }
    printf("Neighbor tables initialized.\n");
    printf("The most neighbors is %i, whereas the max allowed is %i.\n",
           most_neighbors, sw.max_neighbors);
  }

  // ----------------------------------------------------------------------------
  // Make sure initial placement is valid
  // ----------------------------------------------------------------------------

  for(int i = 0; i < sw.N; i++) {
    for(int j = 0; j < i; j++) {
      if (overlap(sw.balls[i], sw.balls[j], sw.len, sw.walls)) {
        print_bad(sw.balls, sw.N, sw.len, sw.walls);
        printf("Error in initial placement: balls are overlapping.\n");
        return 253;
      }
    }
  }

  fflush(stdout);

  // ----------------------------------------------------------------------------
  // Initialization of cell
  // ----------------------------------------------------------------------------

  sw.initialize_translation_distance();

  // --------------------------------------------------------------------------
  // end initilization routine.
  // --------------------------------------------------------------------------

  // ----------------------------------------------------------------------------
  // Generate info to put in save files
  // ----------------------------------------------------------------------------

  mkdir(data_dir, 0777); // create save directory

  char *headerinfo = new char[4096];
  sprintf(headerinfo,
          "# version: %s\n"
          "# cell dimensions: (%g, %g, %g)\n"
          "# walls: %i\n"
          "# de_g: %g\n"
          "# seed: %li\n"
          "# N: %i\n"
          "# R: %f\n"
          "# well_width: %g\n"
          "# translation_scale: %g\n"
          "# neighbor_scale: %g\n"
          "# scaling factor: %g\n"
          "# ff: %g\n"
          "# ff_small: %g\n",
          version_identifier(),
          sw.len[0], sw.len[1], sw.len[2], sw.walls, de_g, seed, sw.N, R,
          well_width, sw.translation_scale, neighbor_scale, scaling_factor, ff, ff_small);

  char *g_fname = new char[1024];
  sprintf(g_fname, "%s/%s.dat", data_dir, filename);

  took("Initialization");

  // ----------------------------------------------------------------------------
  // MAIN PROGRAM LOOP
  // ----------------------------------------------------------------------------

  sw.moves.total = 0;
  sw.moves.working = 0;
  sw.iteration = 0;

  // tracking for free energy
  int current_valid_run = 0;
  int current_failed_run = 0;
  int longest_valid_run = 0;
  int longest_failed_run = 0;
  int total_checks_of_small_cell = 0;
  int total_valid_small_checks = 0;
  int total_failed_small_checks = 0;
  int valid_runs = 0;
  int failed_runs = 0;
  double average_valid_run = 0;
  double average_failed_run = 0;

  // Reset energy histogram and sample counts
  for(int i = 0; i < sw.energy_levels; i++){
    sw.energy_histogram[i] = 0;
    sw.pessimistic_samples[i] = 0;
    sw.optimistic_samples[i] = 0;
  }

  do {
    // ---------------------------------------------------------------
    // Move each ball once, add to energy histogram
    // ---------------------------------------------------------------
    for(int i = 0; i < sw.N; i++){
      sw.move_a_ball();
    }

    // just hacking stuff in to see what works
    // do the small bit every 100 n^2 iterations for now
    if (sw.iteration % small_cell_check_period == 0) {
      total_checks_of_small_cell++;

      if(overlap_in_small_cell(sw,  scaling_factor)){
        total_failed_small_checks++;
        
        if (current_failed_run == 0){
          // then valid run just ended so record it
          valid_runs++;
          if(current_valid_run > longest_valid_run)
            longest_valid_run = current_valid_run;
          average_valid_run = average_valid_run + (current_valid_run - average_valid_run)/valid_runs;
          current_valid_run = 0;
        }

        current_failed_run++;

        if(debug){printf("%i - false\n", current_failed_run);}
      }
      else{
        total_valid_small_checks++;

        if (current_valid_run == 0){
          // then failed run just ended so record it
          failed_runs++;
          if(current_failed_run > longest_failed_run)
            longest_failed_run = current_failed_run;
          average_failed_run = average_failed_run + (current_failed_run - average_failed_run)/failed_runs;
          current_failed_run = 0;
        }

        current_valid_run++;

        if(debug){printf("%i - true\n", current_valid_run);}
      }
    }

    // ---------------------------------------------------------------
    // Save to file
    // ---------------------------------------------------------------

    if (time_to_save() || valid_runs == simulation_runs) {
      const clock_t now = clock();
      const double secs_done = double(now)/CLOCKS_PER_SEC;
      const int seconds = int(secs_done) % 60;
      const int minutes = int(secs_done / 60) % 60;
      const int hours = int(secs_done / 3600) % 24;
      const int days = int(secs_done / 86400);
      const long percent_done = 100*valid_runs/simulation_runs;
      printf("Saving data after %i days, %02i:%02i:%02i, %li iterations (%ld%%) "
             "complete.\n", days, hours, minutes, seconds, sw.iteration,
             percent_done);
      fflush(stdout);

      char *countinfo = new char[4096];
      sprintf(countinfo,
              "# iterations: %li\n"
              "# working moves: %li\n"
              "# total moves: %li\n"
              "# acceptance rate: %g\n"
              "# longest failed run: %i\n"
              "# longest valid run: %i\n"
              "# total checks of small cell: %i\n"
              "# total failed small checks: %i\n"
              "# total valid small checks: %i\n"
              "# valid runs: %i\n"
              "# failed runs: %i\n"
              "# average valid run: %g\n"
              "# average failed run: %g\n\n",
              sw.iteration, sw.moves.working, sw.moves.total,
              double(sw.moves.working)/sw.moves.total, longest_failed_run,
              longest_valid_run, total_checks_of_small_cell, total_failed_small_checks,
              total_valid_small_checks, valid_runs, failed_runs, average_valid_run, average_failed_run);

      // Save data
      if(!sw.walls){
        FILE *g_out = fopen((const char *)g_fname, "w");
        fprintf(g_out, "%s", headerinfo);
        fprintf(g_out, "%s", countinfo);
        fclose(g_out);
      }

      delete[] countinfo;
    }
  } while (valid_runs < simulation_runs);
  // ----------------------------------------------------------------------------
  // END OF MAIN PROGRAM LOOP
  // ----------------------------------------------------------------------------

  for (int i=0; i<sw.N; i++) {
    delete[] sw.balls[i].neighbors;
  }
  delete[] sw.balls;
  delete[] sw.ln_energy_weights;
  delete[] sw.energy_histogram;

  delete[] sw.transitions_table;

  delete[] sw.walkers_up;

  delete[] sw.pessimistic_observation;
  delete[] sw.pessimistic_samples;
  delete[] sw.optimistic_samples;

  delete[] headerinfo;
  delete[] g_fname;

  delete[] data_dir;
  delete[] filename;
  delete[] filename_suffix;

  return 0;
}
// ------------------------------------------------------------------------------
// END OF MAIN
// ------------------------------------------------------------------------------

static bool overlap_in_small_cell(sw_simulation &sw, double scaling_factor){
  double scaled_len[3];

  for(int i=0; i < 3; i++){
    scaled_len[i] = scaling_factor * sw.len[i];
  }

  for(int i=0; i<sw.N; i++){
    for (int j=i+1; j<sw.N; j++) {
      // copy pasting from overlap() in square-well.cpp for now
      // contemplated adding a scaling parametor to overlap(), but decided on this for now.
      const vector3d ab = periodic_diff(scaling_factor * sw.balls[i].pos, scaling_factor * sw.balls[j].pos,
                                        scaled_len, sw.walls);
      if (ab.normsquared() < sqr(sw.balls[i].R + sw.balls[j].R)) {
        return true;
      }
    }
  }
  return false;
}

inline void print_all(const ball *p, int N) {
  for (int i = 0; i < N; i++) {
    char *pos = new char[1024];
    p[i].pos.tostr(pos);
    printf("%4i: R: %4.2f, %i neighbors: ", i, p[i].R, p[i].num_neighbors);
    for(int j = 0; j < min(10, p[i].num_neighbors); j++)
      printf("%i ", p[i].neighbors[j]);
    if (p[i].num_neighbors > 10)
      printf("...");
    printf("\n      pos:          %s\n", pos);
    delete[] pos;
  }
  printf("\n");
  fflush(stdout);
}

inline void print_one(const ball &a, int id, const ball *p, int N,
                      double len[3], int walls) {
  char *pos = new char[1024];
  a.pos.tostr(pos);
  printf("%4i: R: %4.2f, %i neighbors: ", id, a.R, a.num_neighbors);
  for(int j=0; j<min(10, a.num_neighbors); j++)
    printf("%i ", a.neighbors[j]);
  if (a.num_neighbors > 10)
    printf("...");
  printf("\n      pos:          %s\n", pos);
  for (int j=0; j<N; j++) {
    if (j != id && overlap(a, p[j], len, walls)) {
      p[j].pos.tostr(pos);
      printf("\t  Overlaps with %i", j);
      printf(": %s\n", pos);
    }
  }
  delete[] pos;
  printf("\n");
  fflush(stdout);
}

inline void print_bad(const ball *p, int N, double len[3], int walls) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < i; j++) {
      if (overlap(p[i], p[j], len, walls)) {
        char *pos = new char[1024];
        p[i].pos.tostr(pos);
        printf("%4i: %s R: %4.2f\n", i, pos, p[i].R);
        for (int j = 0; j < i; j++) {
          if (overlap(p[i], p[j], len, walls)) {
            p[j].pos.tostr(pos);
            printf("\t  Overlaps with %i", j);
            printf(": %s\n", pos);
          }
        }
        delete[] pos;
        break;
      }
    }
  }
  fflush(stdout);
}

inline void check_neighbor_symmetry(const ball *p, int N) {
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < p[i].num_neighbors; j++) {
      const int k = p[i].neighbors[j];
      bool is_neighbor = false;
      for (int l = 0; l < p[k].num_neighbors; l++) {
        if (p[k].neighbors[l] == i) {
          is_neighbor = true;
          break;
        }
      }
      if(!is_neighbor) {
        printf("NEIGHBOR TABLE ERROR: %i has %i as a neighbor, but %i does "
               "not reciprocate!!!\n", i, k, k);
      }
    }
  }
}

static void took(const char *name) {
  assert(name); // so it'll count as being used...
  static clock_t last_time = clock();
  clock_t t = clock();
  double seconds = (t-last_time)/double(CLOCKS_PER_SEC);
  if (seconds > 120) {
    printf("%s took %.0f minutes and %g seconds.\n", name, seconds/60,
           fmod(seconds,60));
  } else {
    printf("%s took %g seconds...\n", name, seconds);
  }
  fflush(stdout);
  last_time = t;
}

void save_locations(const ball *p, int N, const char *fname, const double len[3],
                    const char *comment) {
  FILE *out = fopen((const char *)fname, "w");
  fprintf(out, "# %s\n", comment);
  fprintf(out, "%g %g %g\n", len[x], len[y], len[z]);
  for(int i = 0; i < N; i++) {
    fprintf(out, "%6.2f %6.2f %6.2f ", p[i].pos[x], p[i].pos[y], p[i].pos[z]);
    fprintf(out, "\n");
  }
  fclose(out);
}
INITBOX *load_cube(char *data_dir){
	  char *fName = new char[4096];
	  sprintf(fName,"%s/baseAtoms.txt",data_dir);
	  FILE *pos_in = fopen((const char *)fName, "r");
	  delete[] fName;
	  if(pos_in==NULL) return NULL;
	  int N;
	  double L;
	  fscanf(pos_in,"L = %lf\nN = %i\n",&L,&N);
	  if(N<0 || N>1000) {fclose(pos_in);return NULL;}
	  double *x=new double[N];
	  double *y=new double[N];
	  double *z=new double[N];
	  for(int n=0;n<N-1;n++){
		  fscanf(pos_in,"%lf %lf %lf\n",&x[n],&y[n],&z[n]);
	  }
	  fscanf(pos_in,"%lf %lf %lf",&x[N-1],&y[N-1],&z[N-1]);
	  fclose(pos_in);
	  INITBOX *box=new INITBOX(L);
	  for(int n=0;n<N;n++) box->addAtom(x[n],y[n],z[n]);
	  delete [] x;
	  delete [] y;
	  delete [] z;
	  return box;
}
bool pack_cube(sw_simulation &sw){
	//assumes you want to pack a cube (see name)
	  INITBOX *box=new INITBOX(sw.len[x],sw.N);
	  if(box->numAtoms!=sw.N){ delete box;return false;}
		  for(int i=0;i<sw.N;i++){
			  sw.balls[i].pos=vector3d(box->list[i].x,box->list[i].y,box->list[i].z);
		  }
		  printf("atoms initialized using rain method\n");
	  delete box;
	  return true;
} 
bool time_to_save() {
  static clock_t output_period = CLOCKS_PER_SEC; // start at outputting every second
  // top out at one hour interval
  clock_t max_output_period = clock_t(CLOCKS_PER_SEC)*60*60;
  static clock_t last_save_time = 0;

  static int iterations = 0;
  static int how_often = 1;
  // clock can be expensive under fac, so this is a heuristic to
  // reduce our use of it.
  if (++iterations % how_often == 0) {
    const clock_t time_now = clock();
    if(time_now-last_save_time > output_period){

      if (output_period < max_output_period/2) output_period *= 2;
      else if (output_period < max_output_period)
        output_period = max_output_period;

      how_often = 1+ iterations/3; // our simple heuristic
      last_save_time = time_now;
      iterations = 0;
      // flushing occasionally will be no problem and can be helpful
      // if we forget.
      fflush(stdout);
      return true;
    }
  }
  return false;
}
