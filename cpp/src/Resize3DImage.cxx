#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

// C++ functions
#include <iostream>
#include <cmath>

// Boost Filesystem library
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
namespace fs = boost::filesystem;

// Command line parser header file
#include <tclap/CmdLine.h>

// ITK files
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkIdentityTransform.h"
#include "itkRecursiveGaussianImageFilter.h"

// entry point for the program
int main(int argc, char** argv)
{
    
    /*******************************/
    /** Command line parser block **/
    /*******************************/
    
    // command line input argument types and variables
    fs::path                            imPath;
    bool                                verbose;
    fs::path                            outImPath;
    std::string                         interpType; // interpolator type
    size_t                              sX, sY, sZ; // output size
    bool                                sigmaSeg3D; // whether to use a very similar blurring to Seg3D's
    
    try {
        
        // Define the command line object, program description message, separator, version
        TCLAP::CmdLine cmd( "resize3DImage: resize a 3D image", ' ', "0.0" );
    
        // input argument: verbosity
        TCLAP::SwitchArg sigmaSeg3DSwitch("", "sigmaSeg3D", "Use similar low-pass blurring as Seg3D's Resample tool", false);
        cmd.add(sigmaSeg3DSwitch);
        
        // input argument: filename of output image
        TCLAP::ValueArg< std::string > outImPathArg("o", "outfile", "Output image filename", false, "", "file");
        cmd.add(outImPathArg);

        // input argument: interpolating type
        TCLAP::ValueArg< std::string > interpTypeArg("i", "interp", "Interpolator type: bspline (default), nn", false, "bspline", "string");
        cmd.add(interpTypeArg);

        // input argument: verbosity
        TCLAP::SwitchArg verboseSwitch("v", "verbose", "Increase verbosity of program output", false);
        cmd.add(verboseSwitch);
    
        // input argument: output size
        TCLAP::UnlabeledValueArg< size_t > sXArg("sx", "Output size X", true, 0, "sx");
        TCLAP::UnlabeledValueArg< size_t > sYArg("sy", "Output size Y", true, 0, "sy");
        TCLAP::UnlabeledValueArg< size_t > sZArg("sz", "Output size Z", true, 0, "sz");
        cmd.add(sXArg);
        cmd.add(sYArg);
        cmd.add(sZArg);

        // input argument: filename of input file
        TCLAP::UnlabeledValueArg< std::string > imPathArg("image", "3D image", true, "", "file");
        cmd.add(imPathArg);
        
        // Parse the command line arguments
        cmd.parse(argc, argv);

        // Get the value parsed by each argument
        imPath = fs::path(imPathArg.getValue());
        outImPath = fs::path(outImPathArg.getValue());
        verbose = verboseSwitch.getValue();
        interpType = interpTypeArg.getValue();
        sX = sXArg.getValue();
        sY = sYArg.getValue();
        sZ = sZArg.getValue();
        sigmaSeg3D = sigmaSeg3DSwitch.getValue();
        
    } catch (const TCLAP::ArgException &e)  // catch any exceptions
    {
        std::cerr << "Error parsing command line: " << std::endl 
        << e.error() << " for arg " << e.argId() << std::endl;
        return EXIT_FAILURE;
    }
    
    /*******************************/
    /** Load input image block    **/
    /*******************************/

    static const unsigned int   Dimension = 3; // volume data dimension (i.e. 3D volumes)
    typedef double              TScalarType; // data type for scalars (e.g. point coordinates)
    
    typedef float                                        FloatPixelType;
    typedef itk::Image< FloatPixelType, 
                        Dimension >                      InputImageType;
    typedef InputImageType::SizeType                     InputSizeType;
    typedef itk::ImageFileReader< InputImageType >       ReaderType;

    // landmark I/O variables
    ReaderType::Pointer                                  imReader;
        
    // image variables
    InputSizeType                                        sizeIn;
    InputImageType::Pointer                              imIn;
    
    try {
        
        // create file readers
        imReader = ReaderType::New();
        
        // read input 3D images
        imReader->SetFileName( imPath.file_string() );
        if ( verbose ) {
            std::cout << "# Input image filename: " << imPath.file_string() << std::endl;
        }
        imReader->Update();
        
        // get input image
        imIn = imReader->GetOutput();
        
        // get image's size
        sizeIn = imIn->GetLargestPossibleRegion().GetSize();

        if ( verbose ) {
            std::cout << "# Input image dimensions: " << sizeIn[0] << "\t" 
                << sizeIn[1] << "\t" << sizeIn[2] << std::endl; 
        }

    } catch( const std::exception &e )  // catch any exceptions
    {
        std::cerr << "Error loading input image: " << std::endl 
        << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    /*******************************/
    /** Smooth image              **/
    /*******************************/

    // [from ITK's /usr/share/doc/insighttoolkit3-examples/examples/Filtering/SubsampleVolume.cxx.gz]

    typedef itk::RecursiveGaussianImageFilter< 
                                  InputImageType,
                                  InputImageType >       GaussianFilterType;

    typedef unsigned short                               UShortPixelType;
    typedef itk::Image< UShortPixelType, 
                        Dimension >                      OutputImageType;
    typedef OutputImageType::SizeType                    OutputSizeType;

    // image variables
    InputImageType::SpacingType                          spacingIn;  
    OutputImageType::Pointer                             imOut;
    OutputSizeType                                       sizeOut;
 

    GaussianFilterType::Pointer                          smootherX;
    GaussianFilterType::Pointer                          smootherY;
    GaussianFilterType::Pointer                          smootherZ;

    try {

        // get parameter values
        spacingIn = imIn->GetSpacing();

        // output size (if command line value is 0, then use input image size)
        if (sX == 0) {sizeOut[0] = sizeIn[0];} else {sizeOut[0] = sX;} 
        if (sY == 0) {sizeOut[1] = sizeIn[1];} else {sizeOut[1] = sY;}
        if (sZ == 0) {sizeOut[2] = sizeIn[2];} else {sizeOut[2] = sZ;}
        
        // instantiate smoother
        smootherX = GaussianFilterType::New();
        smootherY = GaussianFilterType::New();
        smootherZ = GaussianFilterType::New();
        
        // set image to smooth
        smootherX->SetInput(imIn);
        smootherY->SetInput(smootherX->GetOutput());
        smootherZ->SetInput(smootherY->GetOutput());
        
        // set standard deviation for smoother 
        double sigmaX = spacingIn[0] * (double)sizeIn[0] / (double)sizeOut[0];
        double sigmaY = spacingIn[1] * (double)sizeIn[1] / (double)sizeOut[1];
        double sigmaZ = spacingIn[2] * (double)sizeIn[2] / (double)sizeOut[2];
        if (sigmaSeg3D) {
            sigmaX *= 0.61;
            sigmaY *= 0.61;
            sigmaZ *= 0.61;
        }

        smootherX->SetSigma(sigmaX);
        smootherY->SetSigma(sigmaY);
        smootherZ->SetSigma(sigmaZ);
        
        // "we instruct each one of the smoothing filters to act along a particular
        // direction of the image, and set them to use normalization across scale space
        // in order to prevent for the reduction of intensity that accompanies the
        // diffusion process associated with the Gaussian smoothing." (ITK's example)
        smootherX->SetDirection(0);
        smootherY->SetDirection(1);
        smootherZ->SetDirection(2);

        smootherX->SetNormalizeAcrossScale(false);
        smootherY->SetNormalizeAcrossScale(false);
        smootherZ->SetNormalizeAcrossScale(false);
        
        
    } catch( const std::exception &e )  // catch exceptions
    {
        std::cerr << "Error smoothing input image: " << std::endl 
        << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    
    /*******************************/
    /** Resize image              **/
    /*******************************/

    typedef itk::IdentityTransform< TScalarType, 
                                  Dimension >            IdentityTransformType;
    typedef itk::ResampleImageFilter<
                  InputImageType, OutputImageType >      ResampleFilterType;
    // cubic spline
    typedef itk::BSplineInterpolateImageFunction< 
                  InputImageType, TScalarType >          BSplineInterpolatorType;
    typedef itk::NearestNeighborInterpolateImageFunction< 
                  InputImageType, TScalarType >          NearestNeighborInterpolatorType;
    typedef itk::InterpolateImageFunction< 
                  InputImageType, TScalarType >          InterpolatorType;
                  

    OutputImageType::SpacingType                         spacingOut;  

    // filters
    IdentityTransformType::Pointer                       transform;
    ResampleFilterType::Pointer                          resampler;
    InterpolatorType::Pointer                            interpolator;

    try {

        // create objects for downsample
        transform = IdentityTransformType::New();
        resampler = ResampleFilterType::New();
        if (interpType == "bspline") {
            interpolator = BSplineInterpolatorType::New();
        } else if (interpType == "nn") {
            interpolator = NearestNeighborInterpolatorType::New();
        } else {
            throw std::string("Invalid interpolator type");
        }
        
        // compute spacing factor in the output image
        for (size_t i = 0; i < Dimension; ++i) { 
            spacingOut[i] = spacingIn[i] * (double)sizeIn[i] / (double)sizeOut[i];
        }

        // set all the bits and pieces that go into the resampler
        resampler->SetInterpolator(interpolator);
        resampler->SetTransform(transform);
        resampler->SetOutputOrigin(imIn->GetOrigin());
        resampler->SetOutputSpacing(spacingOut);
        resampler->SetSize(sizeOut);
        resampler->SetInput(smootherZ->GetOutput()); 
        
        // rotate image
        resampler->Update();
        imOut = resampler->GetOutput();

        if ( verbose ) {
            std::cout << "# Output Image dimensions: " << sizeOut[0] << "\t" 
                << sizeOut[1] << "\t" << sizeOut[2] << std::endl; 
        }
        
    } catch( const std::exception &e )  // catch exceptions
    {
        std::cerr << "Error resizing input image: " << std::endl 
        << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch( const std::string &e )  // catch exceptions
    {
        std::cerr << "Error resizing input image: " << std::endl 
        << e << std::endl;
        return EXIT_FAILURE;
    }

    /*******************************/
    /** Output block              **/
    /*******************************/

    typedef itk::ImageFileWriter< OutputImageType >      WriterType;

    // I/O variables
    WriterType::Pointer                                  writer;
        
    try {     

        // create writer object        
        writer = WriterType::New();
        
        // create a filename for the output image by appending 
        // "rotated" to the input image filename, if none is
        // provided explicitely in the command line
        if ( outImPath.empty() ) {
            outImPath = imPath.branch_path() 
            / fs::path(fs::basename(imPath) + "-resized" 
            + fs::extension(imPath));
        }

        if ( verbose ) {
            std::cout << "# Output filename: " << outImPath.file_string() << std::endl;
        }
        
        // write output file
        writer->SetInput(imOut);
        writer->SetFileName(outImPath.file_string());
        writer->SetUseCompression(true);
        writer->Update();
           
    } catch( const std::exception &e )  // catch any exceptions
    {
        std::cerr << "Error writing output image: " << std::endl 
        << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    /*******************************/
    /** End of program            **/
    /*******************************/
    
    return EXIT_SUCCESS; 
}
        