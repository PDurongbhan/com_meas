/*----------------------------------------------------------------------------\
|  files: COM_MEAS                                                            
||                                                                             
||  description:                                                               
||   Finds the centre of mass of two solid bone masks, calculates the distance 
||   between the two centres of mass and the angles between the connecting     
||   line and all three axes						                              
||									                                          
||  input:  two solid bone masks		            		                      
||  output: text file containing distance between centre of masses and angles  
||          between the connecting line and all three axes 	        	      
||                                                                             
||  author:                                                                    
||                                                                             
||  date: 10.08.2008                                                           
||                                                                             
||  modified:  06.10.2014, Patrick Weber, on VMS at ETH using Aimpack
||             27.06.2019, Pholpat Durongbhan, on VMS at UoM using Aimpack
||             19.01.2026, Pholpat Durongbhan, on Windows at UoM using AimIO						
|\----------------------------------------------------------------------------*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "AimIO/AimIO.h"
//#include "COM_MEAS.hxx"
#define PROC_LOG_STRING \
"!                                                                               "\
"!  Processing Log                                                               "\
"!                                                                               "\
"!-------------------------------------------------------------------------------"

#define USAGE \
"USAGE: COM_MEAS file-in file-in2"

void COM_MEAS(AimIO::AimFile *in1Aim, AimIO::AimFile *in2Aim);
int mini(int a, int b);
float distance(int x,int y,int z,int xx, int yy,int zz);

int main(int argc, char *argv[]){

	int status;
	if(argc != 3) {
		printf(USAGE);
		exit(0);
	}
	char* infile1 = argv[1];
	char* infile2 = argv[2];

	// Create AimFile object.
	AimIO::AimFile in1Aim;
	AimIO::AimFile in2Aim;

	// Read header.
	in1Aim.filename = infile1;
	in1Aim.ReadImageInfo();
	
	in2Aim.filename = infile2;
	in2Aim.ReadImageInfo();
	
	COM_MEAS(&in1Aim, &in2Aim);

	return(0);
}
//-----------------------------------------------------------------------------// function COM_MEAS//-----------------------------------------------------------------------------
void COM_MEAS(AimIO::AimFile *in1Aim, AimIO::AimFile *in2Aim) {
	// Create a buffer for the image 1 data.
	assert (in1Aim->buffer_type == AimIO::AimFile::AIMFILE_TYPE_CHAR);
	size_t size1 = long_product (in1Aim->dimensions);
	std::vector<char> image_data1 (size1);

	// Read the image 1 data.
	in1Aim->ReadImageData (image_data1.data(), size1);
	printf("\nRead Aim 1 image data: completed\n");
	
	// Read the image 1 header
	const int64_t dimx1 = in1Aim->dimensions[0];  // Num rows is a compile-time constant
	const int64_t dimy1 = in1Aim->dimensions[1];  // Num columns is a compile-time constant
   	const int64_t dimz1 = in1Aim->dimensions[2];  // Num slices is a compile-time constant
	const int64_t offsetx1 = in1Aim->position[0];
	const int64_t offsety1 = in1Aim->position[1];
	const int64_t offsetz1 = in1Aim->position[2];
	const float elsize1 = in1Aim->element_size[0];
	printf("\nRead Aim 1 image header: completed\n");

	// Create a buffer for the image 2 data.
	assert (in2Aim->buffer_type == AimIO::AimFile::AIMFILE_TYPE_CHAR);
	size_t size2 = long_product (in2Aim->dimensions);
	std::vector<char> image_data2 (size2);
	
	// Read the image 2 data.
	in2Aim->ReadImageData (image_data2.data(), size2);
	printf("\nRead Aim 2 image data: completed\n");

	// Read the image 2 header
	const int64_t dimx2 = in2Aim->dimensions[0];  // Num rows is a compile-time constant
	const int64_t dimy2 = in2Aim->dimensions[1];  // Num columns is a compile-time constant
   	const int64_t dimz2 = in2Aim->dimensions[2];  // Num slices is a compile-time constant
	const int64_t offsetx2 = in2Aim->position[0];
	const int64_t offsety2 = in2Aim->position[1];
	const int64_t offsetz2 = in2Aim->position[2];
	const float elsize2 = in2Aim->element_size[0];
	printf("\nRead Aim 2 image header: completed\n");

	/* Used for debugging
	// Examine some header values.
	std::cout << "The dimensions of Aim 1 are " << in1Aim->dimensions << "\n";
	std::cout << "The position of Aim 1 is " << in1Aim->position << "\n";
	std::cout << "The offset of Aim 1 is " << in1Aim->offset << "\n";
	std::cout << "The dimension x of Aim 1 is " << dimx1 << "\n";
	std::cout << "The dimension y of Aim 1 is " << dimy1 << "\n";
	std::cout << "The dimension z of Aim 1 is " << dimz1 << "\n";
	std::cout << "The position x of Aim 1 is " << offsetx1 << "\n";
	std::cout << "The position y of Aim 1 is " << offsety1 << "\n";
	std::cout << "The position z of Aim 1 is " << offsetz1 << "\n";
	std::cout << "The elsize of Aim 1 is " << elsize1 << "\n";
	std::cout << "The size Aim 1 is " << size1 << "\n";
	
	
	// Examine some header values.
	std::cout << "The dimensions of Aim 2 are " << in2Aim->dimensions << "\n";
	std::cout << "The position of Aim 2 is " << in2Aim->position << "\n";
	std::cout << "The offset of Aim 2 is " << in2Aim->offset << "\n";
	std::cout << "The dimension x of Aim 2 is " << dimx2 << "\n";
	std::cout << "The dimension y of Aim 2 is " << dimy2 << "\n";
	std::cout << "The dimension z of Aim 2 is " << dimz2 << "\n";
	std::cout << "The position x of Aim 2 is " << offsetx2 << "\n";
	std::cout << "The position y of Aim 2 is " << offsety2 << "\n";
	std::cout << "The position z of Aim 2 is " << offsetz2 << "\n";
	std::cout << "The elsize of Aim 2 is " << elsize2 << "\n";
	std::cout << "The size Aim 2 is " << size2 << "\n";
	*/
	
	int64_t i,j,k;
	i=0;
	//find COM
	int64_t offx1=0;
	int64_t offy1=0;
	int64_t offz1 =0;
	int64_t offx2=0;
	int64_t offy2=0;
	int64_t offz2 =0;
	int64_t count1=0;
	int64_t count2=0;
	
	// go over all image voxels and search for object voxels     	
	for (k = 0; k < dimz1; ++k) {
		for (j = 0; j < dimy1; ++j) {
			for (i = 0; i < dimx1; ++i) { // search for 6 neighborhood defined borders				
				if ((image_data1[k*dimx1*dimy1+j*dimx1+i] == 127) ) {
					offx1=offx1+i;
					offy1=offy1+j;
					offz1=offz1+k;					
					count1=count1+1;					
				}
			} // end i
		} // end j
	} // end k

	offx1=(offx1/count1)+offsetx1;
	offy1=(offy1/count1)+offsety1;
	offz1=(offz1/count1)+offsetz1;        
	for (k = 0; k < dimz2; ++k) {
		for (j = 0; j < dimy2; ++j) {
			for (i = 0; i < dimx2; ++i) { // search for 6 neighborhood defined borders
				if ((image_data2[k*dimx2*dimy2+j*dimx2+i] == 127) ) {
					offx2=offx2+i;
					offy2=offy2+j;
					offz2=offz2+k;
					count2=count2+1;
				}			
			}// end i		
		}// end j    
	}// end k

	offx2=(offx2/count2)+offsetx2;
	offy2=(offy2/count2)+offsety2;
	offz2=(offz2/count2)+offsetz2;

	float dist,anglex, angley,anglez;
	dist=distance(offx1,offy1,offz1,offx2,offy2,offz2);
	anglex=57.3*acos((offx2-offx1)/dist);
	angley=57.3*acos((offy2-offy1)/dist);
	anglez=57.3*acos((offz2-offz1)/dist);
	
	/* Used for debugging
	printf("\nInaim reset: ");
	std::cout << "The center of mass of the reset aim is : " << offx1 << ", " << offy1 << ", " << offz1 << std::endl;
	
	std::cout << "Count1 is : " << count1 << std::endl;
	std::cout << "Offx1 is : " << offx1 << std::endl;
	std::cout << "Offy1 is : " << offy1 << std::endl;
	std::cout << "Offz1 is : " << offz1 << std::endl;
    
	std::cout << "Count2 is : " << count2 << std::endl;
	std::cout << "Offx2 is : " << offx2 << std::endl;
	std::cout << "Offy2 is : " << offy2 << std::endl;
	std::cout << "Offz2 is : " << offz2 << std::endl;
	std::cout << "The center of mass of the distal mask is : " << offx1 << ", " << offy1 << ", " << offz1 << std::endl;
	std::cout << "The center of mass of the proximal mask is : " << offx2 << ", " << offy2 << ", " << offz2 << std::endl;
	*/
	
	std::cout << std::endl << "The distance between the center of masses is : " << dist*elsize1 << std::endl;
	std::cout << "The inclination with respect to x: " << anglex << " y: " << angley << " z: " << anglez << std::endl;

	FILE* file;
	file = fopen("COMMS_RESULT.CSV","w");
	printf("File opened successfully.\n");
	fprintf(file,"Distance between COMS,%f,",(dist*elsize1));
	fprintf(file,"Angle x,%f,",anglex);
	fprintf(file,"Angle y,%f,",angley);
	fprintf(file,"Angle z,%f",anglez);
	fclose(file);
	printf("File closed successfully.\n");
}
//end com_meas

int mini(int a,int b) {
		if(a>b) return(b);
		else return(a);
		}

float distance(int x,int y,int z,int xx, int yy,int zz) {
	return(sqrt((xx-x)*(xx-x)+(yy-y)*(yy-y)+(zz-z)*(zz-z)));
}
