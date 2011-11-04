#include "kami.h"
#include "cl.h"

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <CL/opencl.h>

namespace klib{
	KLCL::KLCL()
	{
		// NVIDIA SDK example code
		err = oclGetPlatformID(&platform);
		//oclErrorString is also from the NVIDIA SDK
		//printf("oclGetPlatformID: %s\n", oclErrorString(err));

		// Get the number of GPU devices available to the platform
		// we should probably expose the device type to the user
		// the other common option is CL_DEVICE_TYPE_CPU
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
		//printf("clGetDeviceIDs (get number of devices): %s\n", oclErrorString(err));


		// Create the device list
		devices = new cl_device_id [numDevices];
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
		//printf("clGetDeviceIDs (create device list): %s\n", oclErrorString(err));

		//create the context
		context = clCreateContext(0, 1, devices, NULL, NULL, &err);

		//for right now we just use the first available device
		//later you may have criteria (such as support for different extensions)
		//that you want to use to select the device
		deviceUsed = 0;

		//create the command queue we will use to execute OpenCL commands
		command_queue = clCreateCommandQueue(context, devices[deviceUsed], 0, &err);

		cl_a = 0;
		cl_b = 0;
		cl_c = 0;

		err = clGetPlatformInfo (platform, CL_PLATFORM_NAME, 1024, &platformName, NULL);

		printf("Initialized CL %s\n", platformName);
	}

	KLCL::~KLCL()
	{
		printf("Releasing OpenCL memory\n");
		if(kernel)clReleaseKernel(kernel); 
		if(program)clReleaseProgram(program);
		if(command_queue)clReleaseCommandQueue(command_queue);

		//need to release any other OpenCL memory objects here
		if(cl_a)clReleaseMemObject(cl_a);
		if(cl_b)clReleaseMemObject(cl_b);
		if(cl_c)clReleaseMemObject(cl_c);

		if(context)clReleaseContext(context);

		if(devices)delete(devices);
		printf("OpenCL memory released\n");
	}

	void KLCL::load(const char* relative_path)
	{
		// Program Setup
		int pl;
		size_t program_length;

		//it loads the contents of the file at the given path
		char* cSourceCL = file_contents(const_cast<char*>(relative_path), &pl);
		//printf("file: %s\n", cSourceCL);
		program_length = (size_t)pl;

		// create the program
		program = clCreateProgramWithSource(context, 1, (const char **) &cSourceCL, &program_length, &err);
		//printf("clCreateProgramWithSource: %s\n", oclErrorString(err));

		compile();

		//Free buffer returned by file_contents
		free(cSourceCL);
	}

	void KLCL::initialize()
	{
		//initialize our kernel from the program
		kernel = clCreateKernel(program, "part1", &err);

		//initialize our CPU memory arrays, send them to the device and set the kernel arguements
		num = 10;
		float *a = new float[num];
		float *b = new float[num];
		for(int i=0; i < num; i++)
		{
			a[i] = 1.0f * (rand()%(i+1));
			b[i] = 1.0f * (rand()%(i+1));
		}
		//our input arrays
		//create our OpenCL buffer for a, copying the data from CPU to the GPU at the same time
		cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float) * num, a, &err);
		//cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float) * num, b, &err);
		//we could do b similar, but you may want to create your buffer and fill it at a different time
		cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * num, NULL, &err);
		//our output array
		cl_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * num, NULL, &err);

		//push our CPU arrays to the GPU
		//    err = clEnqueueWriteBuffer(command_queue, cl_a, CL_TRUE, 0, sizeof(float) * num, a, 0, NULL, &event);
		//    clReleaseEvent(event); //we need to release events in order to be completely clean (has to do with openclprof)
		//
		//push b's data to the GPU
		err = clEnqueueWriteBuffer(command_queue, cl_b, CL_TRUE, 0, sizeof(float) * num, b, 0, NULL, &event);
		clReleaseEvent(event);


		//set the arguements of our kernel
		err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &cl_a);
		err  = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &cl_b);
		err  = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &cl_c);
		//Wait for the command queue to finish these commands before proceeding
		clFinish(command_queue);

		//clean up allocated space.
		delete[] a;
		delete[] b;

		//for now we make the workgroup size the same as the number of elements in our arrays
		workGroupSize[0] = num;
	}

	void KLCL::execute()
	{
		//execute the kernel
		err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &event);
		clReleaseEvent(event);
		clFinish(command_queue);

		//lets check our calculations by reading from the device memory and printing out the results
		float c_done[10];
		err = clEnqueueReadBuffer(command_queue, cl_c, CL_TRUE, 0, sizeof(float) * num, &c_done, 0, NULL, &event);
		clReleaseEvent(event);

		for(int i=0; i < num; i++)
		{
			printf("c_done[%d] = %g\n", i, c_done[i]);
		}
	}

	void KLCL::compile()
	{
		// build the program
		err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
		//if(err != CL_SUCCESS){
		cl_build_status build_status;
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);

		char *build_log;
		size_t ret_val_size;
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);

		build_log = new char[ret_val_size+1];
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
		build_log[ret_val_size] = '\0';
		//}
	}
}