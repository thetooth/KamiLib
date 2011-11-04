#pragma once

#include "kami.h"

/*
Model.h

Abstract base class for a model. The specific extended class will render the given model. 

Author:	Brett Porter
Email: brettporter@yahoo.com
Website: http://www.geocities.com/brettporter/
Copyright (C)2000, Brett Porter. All Rights Reserved.

This file may be used only as long as this copyright notice remains intact.
*/

namespace klib{
	class Model
	{
	public:
		//	Mesh
		struct Mesh
		{
			int m_materialIndex;
			int m_numTriangles;
			int *m_pTriangleIndices;
		};

		//	Material properties
		struct Material
		{
			float m_ambient[4], m_diffuse[4], m_specular[4], m_emissive[4];
			float m_shininess;
			GLuint m_texture;
			char *m_pTextureFilename;
		};

		//	Triangle structure
		struct Triangle
		{
			float m_vertexNormals[3][3];
			float m_s[3], m_t[3];
			int m_vertexIndices[3];
		};

		//	Vertex structure
		struct Vertex
		{
			char m_boneID;	// for skeletal animation
			float m_location[3];
		};

	public:
		/*	Constructor. */
		Model();

		/*	Destructor. */
		virtual ~Model();

		/*	
		Load the model data into the private variables. 
		filename			Model filename
		*/
		virtual int loadModelData(KLGL &gc, const char *filename) = 0;

		/*
		Draw the model.
		*/
		void draw();

		/*
		Called if OpenGL context was lost and we need to reload textures, display lists, etc.
		*/
		void reloadTextures(KLGL &gc);

	protected:
		//	Meshes used
		int m_numMeshes;
		Mesh *m_pMeshes;

		//	Materials used
		int m_numMaterials;
		Material *m_pMaterials;

		//	Triangles used
		int m_numTriangles;
		Triangle *m_pTriangles;

		//	Vertices Used
		int m_numVertices;
		Vertex *m_pVertices;
	};

	// Milkshape 3D loader
	class MilkshapeModel : public Model
	{
	public:
		/*	Constructor. */
		MilkshapeModel();

		/*	Destructor. */
		virtual ~MilkshapeModel();

		/*	
		Load the model data into the private variables. 
		filename			Model filename
		*/
		virtual int loadModelData(KLGL &gc, const char *filename);
	};
}