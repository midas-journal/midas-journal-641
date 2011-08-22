#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkBranchDecompositionFilter.h"

template<class TFilter>
class CommandProgressUpdate : public itk::Command 
{
public:
  typedef  CommandProgressUpdate    Self;
  typedef  itk::Command             Superclass;
  typedef  itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );
  
  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    const TFilter * filter = dynamic_cast< const TFilter * >(object);
    if( !(itk::ProgressEvent().CheckEvent( &event )) )  return;
    std::cout << (int)(100 * filter->GetProgress()) << "% completed\r" << std::flush;
  }

protected:
  CommandProgressUpdate(){};
};

int main( int argc, char* argv[] )
{
  if( argc < 7 )
  {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputImage outputImage inputJunctioninfoTXT outputBranchinfoTXT inner outer" << std::endl;
    std::cerr << "Example: " << std::endl;
    std::cerr << argv[0] << " input.mha output.mha jcinfo.txt brinfo.txt 2.5 3.0" << std::endl;
    return EXIT_FAILURE;
  }
  
  typedef itk::Image<unsigned short, 2>                         ImageType;
  typedef itk::ImageFileReader<ImageType>                       ReaderType;
  typedef itk::BranchDecompositionFilter<ImageType>             DecomposerType;
  typedef itk::ImageFileWriter<DecomposerType::OutputImageType> WriterType;

  char *inputfilename = argv[1];
  char *outputfilename = argv[2];
  char *jcinfofilename = argv[3];
  char *brinfofilename = argv[4];
  float inner = atof(argv[5]);
  float outer = atof(argv[6]);

  ImageType::Pointer inputImage;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputfilename );
  try 
  { 
    reader->Update();
    inputImage = reader->GetOutput();
  } 
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ExceptionObject caught !" << std::endl; 
    std::cerr << err << std::endl; 
    return EXIT_FAILURE;
  }

  // Read junction info file
  DecomposerType::JCLabelMapType jcLabelMap;
  std::ifstream jcinfo(jcinfofilename);
  if(!jcinfo.is_open())
  {
    std::cerr << "Cannot open " << jcinfofilename << " to read!" << std::endl;
    return EXIT_FAILURE;
  }
  jcinfo.ignore(256, '\n');
  long jcLabel;
  DecomposerType::IndexType jcIndex;
  float jcRadius;
  while( jcinfo >> jcLabel >> jcIndex[0] >> jcIndex[1] >> jcRadius)
  {
    DecomposerType::JCLabelPairType jclPair(jcIndex, jcRadius);
    jcLabelMap[jcLabel] = jclPair;
  }
  jcinfo.close();

  // Branch decomposition with a progress observer
  typedef CommandProgressUpdate<DecomposerType> CommandType;
  CommandType::Pointer observer = CommandType::New();
  DecomposerType::Pointer decomposer = DecomposerType::New();
  decomposer->SetInnerRadius(inner);
  decomposer->SetOuterRadius(outer);
  decomposer->SetMinNumberOfPixel(6);
  decomposer->SetJCLabelMap(&jcLabelMap);
  decomposer->SetInput(reader->GetOutput());
  decomposer->AddObserver(itk::ProgressEvent(), observer);
  decomposer->Update();
  std::cout << std::endl;

  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputfilename );
  writer->SetInput( decomposer->GetOutput() );
  try 
  { 
    writer->Update(); 
  } 
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ExceptionObject caught !" << std::endl; 
    std::cerr << err << std::endl; 
    return EXIT_FAILURE;
  }
  
  // Output branch infomation in TXT file and write them as spheres in VTP file
  std::ofstream brinfo(brinfofilename);
  if(!brinfo.is_open())
  {
    std::cerr << "Cannot open " << brinfofilename << " to write!" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "brLabel connected_jcLabels" << std::endl;
    brinfo << "brLabel connected_jcLabels" << std::endl;
  }
  const DecomposerType::BRJCConnectMapType& brjcConnectMap = decomposer->GetBRJCConnectMap();
  DecomposerType::BRJCConnectMapType::const_iterator bjiter;
  DecomposerType::BRJCSetType::const_iterator jciter;
  for(bjiter = brjcConnectMap.begin(); bjiter != brjcConnectMap.end(); ++bjiter)
  {
    std::cout << bjiter->first << "  ";
    brinfo << bjiter->first << "  ";
    for(jciter = bjiter->second.begin(); jciter != bjiter->second.end(); ++jciter)
    {
      std::cout << *jciter << " ";
      brinfo << *jciter << " ";
    }
    std::cout << std::endl;
    brinfo << std::endl;
  }
  brinfo.close();

  return EXIT_SUCCESS;
}
