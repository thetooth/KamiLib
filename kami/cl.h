#pragma once

#include "kami.h"

#include <CL/opencl.h>

namespace klib{
	inline cl_int oclGetPlatformID(cl_platform_id* clSelectedPlatformID)
	{
		char chBuffer[1024];
		cl_uint num_platforms;
		cl_platform_id* clPlatformIDs;
		cl_int ciErrNum;
		*clSelectedPlatformID = NULL;
		cl_uint i = 0;

		// Get OpenCL platform count
		ciErrNum = clGetPlatformIDs (0, NULL, &num_platforms);
		if (ciErrNum != CL_SUCCESS)
		{
			//shrLog(" Error %i in clGetPlatformIDs Call !!!\n\n", ciErrNum);
			printf(" Error %i in clGetPlatformIDs Call !!!\n\n", ciErrNum);
			return -1000;
		}
		else
		{
			if(num_platforms == 0)
			{
				//shrLog("No OpenCL platform found!\n\n");
				printf("No OpenCL platform found!\n\n");
				return -2000;
			}
			else
			{
				// if there's a platform or more, make space for ID's
				if ((clPlatformIDs = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id))) == NULL)
				{
					//shrLog("Failed to allocate memory for cl_platform ID's!\n\n");
					printf("Failed to allocate memory for cl_platform ID's!\n\n");
					return -3000;
				}

				// get platform info for each platform and trap the NVIDIA platform if found
				ciErrNum = clGetPlatformIDs (num_platforms, clPlatformIDs, NULL);
				for(i = 0; i < num_platforms; ++i)
				{
					ciErrNum = clGetPlatformInfo (clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, NULL);
					if(ciErrNum == CL_SUCCESS)
					{
						if(strstr(chBuffer, "NVIDIA") != NULL)
						{
							*clSelectedPlatformID = clPlatformIDs[i];
							break;
						}
					}
				}

				// default to zeroeth platform if NVIDIA not found
				if(*clSelectedPlatformID == NULL)
				{
					//shrLog("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n\n");
					//printf("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n\n");
					printf("selected platform: %d\n", 0);
					*clSelectedPlatformID = clPlatformIDs[0];
				}

				free(clPlatformIDs);
			}
		}

		return CL_SUCCESS;
	}
	// Helper function to get error string
	inline const char* oclErrorString(cl_int error)
	{
		static const char* errorString[] = {
			"CL_SUCCESS",
			"CL_DEVICE_NOT_FOUND",
			"CL_DEVICE_NOT_AVAILABLE",
			"CL_COMPILER_NOT_AVAILABLE",
			"CL_MEM_OBJECT_ALLOCATION_FAILURE",
			"CL_OUT_OF_RESOURCES",
			"CL_OUT_OF_HOST_MEMORY",
			"CL_PROFILING_INFO_NOT_AVAILABLE",
			"CL_MEM_COPY_OVERLAP",
			"CL_IMAGE_FORMAT_MISMATCH",
			"CL_IMAGE_FORMAT_NOT_SUPPORTED",
			"CL_BUILD_PROGRAM_FAILURE",
			"CL_MAP_FAILURE",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"CL_INVALID_VALUE",
			"CL_INVALID_DEVICE_TYPE",
			"CL_INVALID_PLATFORM",
			"CL_INVALID_DEVICE",
			"CL_INVALID_CONTEXT",
			"CL_INVALID_QUEUE_PROPERTIES",
			"CL_INVALID_COMMAND_QUEUE",
			"CL_INVALID_HOST_PTR",
			"CL_INVALID_MEM_OBJECT",
			"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
			"CL_INVALID_IMAGE_SIZE",
			"CL_INVALID_SAMPLER",
			"CL_INVALID_BINARY",
			"CL_INVALID_BUILD_OPTIONS",
			"CL_INVALID_PROGRAM",
			"CL_INVALID_PROGRAM_EXECUTABLE",
			"CL_INVALID_KERNEL_NAME",
			"CL_INVALID_KERNEL_DEFINITION",
			"CL_INVALID_KERNEL",
			"CL_INVALID_ARG_INDEX",
			"CL_INVALID_ARG_VALUE",
			"CL_INVALID_ARG_SIZE",
			"CL_INVALID_KERNEL_ARGS",
			"CL_INVALID_WORK_DIMENSION",
			"CL_INVALID_WORK_GROUP_SIZE",
			"CL_INVALID_WORK_ITEM_SIZE",
			"CL_INVALID_GLOBAL_OFFSET",
			"CL_INVALID_EVENT_WAIT_LIST",
			"CL_INVALID_EVENT",
			"CL_INVALID_OPERATION",
			"CL_INVALID_GL_OBJECT",
			"CL_INVALID_BUFFER_SIZE",
			"CL_INVALID_MIP_LEVEL",
			"CL_INVALID_GLOBAL_WORK_SIZE",
		};

		const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

		const int index = -error;

		return (index >= 0 && index < errorCount) ? errorString[index] : "";

	}
	class KLCL {
	public:

		//These are arrays we will use in this tutorial
		cl_mem cl_a;
		cl_mem cl_b;
		cl_mem cl_c;
		int num;    //the size of our arrays


		size_t workGroupSize[1]; //N dimensional array of workgroup size we must pass to the kernel

		//default constructor initializes OpenCL context and automatically chooses platform and device
		KLCL();
		//default destructor releases OpenCL objects and frees device memory
		~KLCL();

		//load an OpenCL program from a file
		//the path is relative to the CL_SOURCE_DIR set in CMakeLists.txt
		void load(const char* relative_path);

		//setup the data for the kernel 
		//these are implemented in part1.cpp (in the future we will make these more general)
		void initialize();

		//execute the kernel
		void execute();

	private:

		//handles for creating an opencl context 
		cl_platform_id platform;

		//device variables
		cl_device_id* devices;
		cl_uint numDevices;
		unsigned int deviceUsed;

		cl_context context;

		cl_command_queue command_queue;
		cl_program program;
		cl_kernel kernel;


		//debugging variables
		cl_int err;
		cl_event event;
		char platformName[1024];

		//buildExecutable is called by loadProgram
		//build runtime executable from a program
		void compile();
	};
}