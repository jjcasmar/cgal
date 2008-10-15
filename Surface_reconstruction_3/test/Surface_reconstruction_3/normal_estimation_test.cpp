// normal_estimation_test.cpp


// ----------------------------------------------------------------------------
// USAGE EXAMPLES
// ----------------------------------------------------------------------------

//----------------------------------------------------------
// Test the normal estimation methods:
// For each input point set, compute and orient its normals.
// No output
//----------------------------------------------------------
// normal_estimation_test points1.xyz points2.xyz...


#include <CGAL/basic.h> // include basic.h before testing #defines

#ifdef CGAL_USE_LAPACK


// CGAL
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Timer.h>
#include <CGAL/Memory_sizer.h>
#include <CGAL/boost/graph/properties.h>

// This package
#include <CGAL/estimate_normals_pca_3.h>
#include <CGAL/estimate_normals_jet_fitting_3.h>
#include <CGAL/orient_normals_minimum_spanning_tree_3.h>
#include <CGAL/Orientable_normal_3.h>
#include <CGAL/Vector_index_property_map.h>
#include <CGAL/IO/surface_reconstruction_read_xyz.h>

// STL stuff
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cassert>


// ----------------------------------------------------------------------------
// Private types
// ----------------------------------------------------------------------------

// kernel
typedef CGAL::Simple_cartesian<float> Kernel;
typedef Kernel::FT FT;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef CGAL::Orientable_normal_3<Kernel> Orientable_normal; // normal vector + orientation

typedef std::vector<Point> PointList;

// ----------------------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------------------

void estimate_normals_pca(const PointList& points, // input point set
                          std::vector<Orientable_normal>& normals, // computed normals
                          unsigned int k) // number of neighbors
{
  std::cerr << "Estimate normals using KNN and point-based PCA (knn="<<k << ")...\n";
  CGAL::Timer task_timer; task_timer.start();

  CGAL::estimate_normals_pca_3(points.begin(),points.end(),std::back_inserter(normals),k);

  long memory = CGAL::Memory_sizer().virtual_size();
  std::cerr << "ok: " << task_timer.time() << " seconds, "
                      << (memory>>20) << " Mb allocated"
                      << std::endl;
}

void estimate_normals_jet_fitting(const PointList& points, // input point set
                                  std::vector<Orientable_normal>& normals, // computed normals
                                  unsigned int k) // number of neighbors)
{
  std::cerr << "Estimate normals using KNN and jet fitting (knn="<<k << ")...\n";
  CGAL::Timer task_timer; task_timer.start();

  CGAL::estimate_normals_jet_fitting_3(points.begin(),points.end(),std::back_inserter(normals),k);

  long memory = CGAL::Memory_sizer().virtual_size();
  std::cerr << "ok: " << task_timer.time() << " seconds, "
                      << (memory>>20) << " Mb allocated"
                      << std::endl;
}

void orient_normals_MST(const PointList& points, // input point set
                        std::vector<Orientable_normal>& normals, // normals to orient
                        unsigned int k) // number of neighbors
{
  std::cerr << "Orient normals using a minimum spanning tree (knn="<<k << ")...\n";
  CGAL::Timer task_timer; task_timer.start();

  // orient_normals_minimum_spanning_tree_3() requires an iterator over points
  // + property maps to access each point's index, position and normal.
  // We use the points index as iterator.
  boost::identity_property_map index_id; // identity
  CGAL::orient_normals_minimum_spanning_tree_3(
         (std::size_t)0, points.size(), // use the points index as iterator
         index_id, // index -> index property map = identity
         boost::make_iterator_property_map(points.begin(), index_id), // index -> position prop. map
         boost::make_iterator_property_map(normals.begin(), index_id), // index -> normal prop. map
         k);


  long memory = CGAL::Memory_sizer().virtual_size();
  std::cerr << "ok: " << task_timer.time() << " seconds, "
                      << (memory>>20) << " Mb allocated"
                      << std::endl;
}


// ----------------------------------------------------------------------------
// main()
// ----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
  std::cerr << "Normal estimation test" << std::endl;

  //***************************************
  // decode parameters
  //***************************************

  // usage
  if (argc-1 == 0)
  {
      std::cerr << "For each input point set, compute and orient its normals.\n";
      std::cerr << "\n";
      std::cerr << "Usage: " << argv[0] << " file1.xyz file2.xyz ..." << std::endl;
      std::cerr << "Input file format is .xyz.\n";
      std::cerr << "No output" << std::endl;
      return EXIT_FAILURE;
  }

  // Options
  const unsigned int k = 10; // # neighbors

  // Accumulated errors
  int accumulated_fatal_err = EXIT_SUCCESS;

  // Process each input file
  for (int arg_index = 1; arg_index <= argc-1; arg_index++)
  {
    CGAL::Timer task_timer; task_timer.start();

    std::cerr << std::endl;

    //***************************************
    // Load mesh/point set
    //***************************************

    // File name is:
    std::string input_filename  = argv[arg_index];

    PointList points;

    std::string extension = input_filename.substr(input_filename.find_last_of('.'));
    if (extension == ".xyz" || extension == ".XYZ")
    {
      // Read the point set file in points[]
      if(!CGAL::surface_reconstruction_read_xyz(input_filename.c_str(),
                                                std::back_inserter(points),
                                                false /*skip normals*/))
      {
        std::cerr << "Error: cannot read file " << input_filename << std::endl;
        accumulated_fatal_err = EXIT_FAILURE;
        continue;
      }
    }
    else
    {
      std::cerr << "Error: cannot read file " << input_filename << std::endl;
      accumulated_fatal_err = EXIT_FAILURE;
      continue;
    }

    // Print status
    long memory = CGAL::Memory_sizer().virtual_size();
    int nb_vertices = points.size();
    std::cerr << "Read file " << input_filename << ": " << nb_vertices << " vertices, "
                                                        << task_timer.time() << " seconds, "
                                                        << (memory>>20) << " Mb allocated"
                                                        << std::endl;
    task_timer.reset();

    //***************************************
    // Check requirements
    //***************************************

    if (nb_vertices == 0)
    {
      std::cerr << "Error: empty file" << std::endl;
      accumulated_fatal_err = EXIT_FAILURE;
      continue;
    }

    //***************************************
    // Compute normals
    //***************************************

    std::vector<Orientable_normal> computed_normals;
    
    // Estimate normals direction
    estimate_normals_pca(points, computed_normals, k);
    computed_normals.clear();
    estimate_normals_jet_fitting(points, computed_normals, k);
      
    // Orient normals
    orient_normals_MST(points, computed_normals, k);
    
    // Check computed normals
    int unoriented_normals = 0;
    std::vector<Orientable_normal>::iterator n;
    for (n = computed_normals.begin(); n != computed_normals.end(); n++)
    {
      // Check unit vector
      Vector v = *n;
      double norm = std::sqrt( v*v );
      assert(norm > 0.99 || norm < 1.01);
        
      // Check orientation
      if ( ! n->is_oriented() )
        unoriented_normals++;
    }
    if (unoriented_normals > 0)
    {
      std::cerr << "Error: " << unoriented_normals << " normal(s) are unoriented\n";
      accumulated_fatal_err = EXIT_FAILURE; // set error and continue
    }

  } // for each input file

  std::cerr << std::endl;

  // Return accumulated fatal error
  std::cerr << "Tool returned " << accumulated_fatal_err << std::endl;
  return accumulated_fatal_err;
}


#else // CGAL_USE_LAPACK


#include <iostream>
#include <cstdlib>

// ----------------------------------------------------------------------------
// Empty main() if LAPACK is not installed
// ----------------------------------------------------------------------------

int main()
{
    std::cerr << "Skip test as LAPACK is not installed" << std::endl;
    return EXIT_SUCCESS;
}


#endif // CGAL_USE_LAPACK

