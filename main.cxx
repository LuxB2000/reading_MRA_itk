/*
 * This simple pipeline reads a folder containing the Magnetic Reasonnance
 * angiography (MRA) generated by Siemens and writes a meta volumetric image.
 * The MRA is assumed to be a set of slices, each taken at a different time.
 * The meta volume will be ordered by content time (0008,0033). Where each 
 * slice of the volume corresponds to a acquisition time.
 *
 * inputs:
 * i: folder path containing the dicom slices,
 * o: meta image file path,
 * v: (optional) verbose option.
 *
 * Examples
 * ./readAngio  -i /some/dicom/folder/path -o /other/path/to/save/as/mha
 * ./readAngio -v  -i /some/dicom/folder/path -o /other/path/to/save/as/mha
 */


#include "../console_tools/color.h"
#include "itkImage.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkGDCMImageIO.h"

#include "itkCastImageFilter.h"
#include "itkImageSeriesReader.h"
#include "itkMetaDataObject.h"

// reader and writer
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

typedef unsigned short                DcmPixelType;
typedef typename itk::Image<DcmPixelType,  3>     DCMImageType;

class DCMDate{
public:
	DCMDate(){}
	DCMDate( double d, DCMImageType::Pointer p){
		date = d;
		ptr = p;
	}
	~DCMDate(){}
	double date;
	DCMImageType::Pointer ptr;
private:
};

// Sort Container by name function
bool sortByAcquisitionDate(const DCMDate &dcm1, const DCMDate &dcm2) {
	return dcm1.date < dcm2.date;
}

int main( int argc, char *argv[] )
{
	std::cout<<" --- "<<argv[0] <<" --- " <<std::endl;
	
	int aFlag = 0, c = 0;
	std::string msg="",defaultStr="";
	std::string inputPath=defaultStr, outputPath=defaultStr;
	bool verbose = false;

	msg="inputs: \n";
	
	// get inputs
	static struct option long_options[] =
	{
		// verbose option
		{"verbose",    no_argument,  0, 'v'},
		// input and output volumes
		{"input",  required_argument,  0, 'i'},
		{"output",  required_argument, 0, 'o'},
		{0,                 0        , 0,  0 }
	};
	/* getopt_long stores the option index here. */
	int option_index = 0;

	while(1){
		c = getopt_long (argc, argv, "i:o:v::",
										 long_options, &option_index);

		// no more options, break the loop
		if( c== -1 ) break;

		switch(c){
			case 'v' :
				verbose = true;
				msg += "\t-v " "\n";
				break;
			// input volume
			case 'i' :
				inputPath = std::string(optarg);
				msg += "\t-i " + inputPath + "\n";
				break;
			// output volume
			case 'o' :
				outputPath = std::string(optarg);
				msg += "\t-o " + outputPath + "\n";
				break;
			default :
				abort();
		}
	}

	if( strcmp(inputPath.c_str(),defaultStr.c_str())==0 ||
			strcmp(outputPath.c_str(),defaultStr.c_str())==0 )
	{
		error("Missing Parameters");
		std::cerr << "Usage: -i InputPath -o OutputPath";
		std::cerr << std::endl;
		exit( EXIT_FAILURE);
	}


	blueMessage(msg);

	//
	// main type definition
	//
  const    unsigned int   Dimension = 3;
  typedef  float PixelType;
	typedef  itk::Image<PixelType, Dimension> ImageType;

	typedef typename itk::Image<float ,  3>   OutputImageType;
	typedef itk::ImageSeriesReader< DCMImageType > DCMReaderType;	
	typedef itk::GDCMSeriesFileNames					NamesGeneratorType;
	typedef itk::GDCMImageIO								ImageIOType;
	typedef std::vector< std::string > SeriesIdContainer;
	typedef std::vector< std::string > FileNamesContainer;

	typedef itk::ImageFileReader< DCMImageType > singleDCMReaderType;
	typedef itk::MetaDataDictionary DictionaryType;
	typedef itk::MetaDataObject< std::string > MetaDataStringType;

	typedef std::string FileNameType;

	// order the angiography capture using Content Time
	const std::string entry_id = "0008|0033";

	ImageIOType::Pointer        dicomIO = ImageIOType::New();
	NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
	DCMReaderType::Pointer reader = DCMReaderType::New();

	reader->SetImageIO( dicomIO );

	// get all the dicom slices in the pointing folder
	nameGenerator->SetDirectory( inputPath ); 

	const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
	std::string seriesIdentifier = seriesUID.begin()->c_str();
	FileNamesContainer fileNames;
	fileNames = nameGenerator->GetFileNames( seriesIdentifier );

	std::vector<DCMDate> dcm_list = std::vector<DCMDate>(fileNames.size());

	// Read all the DCM files and find the content time
	for( int i=0; i<fileNames.size(); i++ ){
		singleDCMReaderType::Pointer f_reader = singleDCMReaderType::New();
		if(verbose) std::cout << fileNames.at(i) << std::endl;
		f_reader->SetFileName( fileNames.at(i));
		// read the slice
		DCMImageType::Pointer im;
		try{
			f_reader->Update();
			im = f_reader->GetOutput();
		}catch(itk::ExceptionObject &err ){
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		// find the acquisition date
		const DictionaryType & dictionary = im->GetMetaDataDictionary();
		DictionaryType::ConstIterator tagItr = dictionary.Find( entry_id );
		if( tagItr == dictionary.End() ){
			std::cerr << ">> Tag " << entry_id;
			std::cerr << " not found in the DICOM header" << std::endl;
			return EXIT_FAILURE;
    }
		MetaDataStringType::ConstPointer entryvalue =
			dynamic_cast<const MetaDataStringType *>( tagItr->second.GetPointer() );
		// if we find the value, create an object
		if( entryvalue ){
			std::string tagkey = tagItr->first;
			std::string labelId;
			bool found = itk::GDCMImageIO::GetLabelFromTag( tagkey, labelId );
			std::string tagvalue = entryvalue->GetMetaDataObjectValue();
			if(found){
				if(verbose){
					std::cout.precision(16);
					std::cout << "(" << tagkey << ") ";
					std::cout << labelId << " = " << atof(tagvalue.c_str());
					std::cout << std::endl;
				}
				dcm_list.at(i) = DCMDate(atof(tagvalue.c_str()), im.GetPointer());
			}
		}else{
			std::cerr << "Entry not found." << std::endl;
			return EXIT_FAILURE;
		}
		im = DCMImageType::New();
	}

	// order all the slice by acquisition time
	std::sort(dcm_list.begin(), dcm_list.end(), sortByAcquisitionDate);
	
	// create an output volume
	DCMImageType::IndexType start; start.Fill(0);
	DCMImageType::PointType origin = dcm_list.at(0).ptr->GetOrigin();
	DCMImageType::Pointer angio = DCMImageType::New();
	DCMImageType::SizeType size = dcm_list.at(0).ptr->GetLargestPossibleRegion().GetSize();
	size[2] = dcm_list.size();

	DCMImageType::RegionType region;
	region.SetSize( size );
	region.SetIndex( start );

	angio->SetRegions(region);
	angio->SetOrigin( origin );
	angio->SetSpacing( dcm_list.at(0).ptr->GetSpacing() );

	try{
		angio->Allocate();
	}catch( itk::ExceptionObject &err ){
		std::cerr << "Error while allocation: ";
		std::cerr << err << std::endl;
	}


	// fill the angio volume
	DCMImageType::IndexType ind, ind2;
	ind2[2] = 0;
	for (std::vector<DCMDate>::size_type i = 0; i < size[2]; ++i){
		ind[2] = i;
				DCMImageType::Pointer dcmi = dcm_list.at(i).ptr;
		for( int x = 0; x<size[0]; x++ ){
			ind[0] = x;
			ind2[0] = x;
			for( int y = 0; y<size[1]; y++ ){
				ind[1] = y;
				ind2[1] = y;
				angio->SetPixel( ind, dcmi->GetPixel(ind2) );
			}
		}
	}
	if(verbose) std::cout << "Create a complete volume containing the MRA." << std::endl;

	// Cast the volume to write
	typedef itk::CastImageFilter<  DCMImageType, OutputImageType >          CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
	caster->SetInput( angio );

	// write the volume
	typedef itk::ImageFileWriter< OutputImageType > MetaWriterType;
	MetaWriterType::Pointer writer = MetaWriterType::New();
	writer->SetFileName( outputPath ); // volume.mha
	writer->SetInput( caster->GetOutput() );
	try{
		writer->Update();
	}catch( itk::ExceptionObject &err){
		std::cerr << "Error while writing the volume as: "
			<< outputPath << std::endl;
		std::cerr << err << std::endl;
		exit( EXIT_FAILURE );
	}
	
	msg = "Output volume wrote with ";
	msg += outputPath;
	finalMessage( msg );
	std::cout << std::endl;
}

