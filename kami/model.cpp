/*
Model.cpp

Abstract base class for a model. The specific extended class will render the given model. 

Author:	Brett Porter
Email: brettporter@yahoo.com
Website: http://www.geocities.com/brettporter/
Copyright (C)2000, Brett Porter. All Rights Reserved.

This file may be used only as long as this copyright notice remains intact.
*/

#include <windows.h>
#include <fstream>

#include "kami.h"
#include "model.h"

namespace klib{
	Model::Model()
	{
		m_numMeshes = 0;
		m_pMeshes = NULL;
		m_numMaterials = 0;
		m_pMaterials = NULL;
		m_numTriangles = 0;
		m_pTriangles = NULL;
		m_numVertices = 0;
		m_pVertices = NULL;
	}

	Model::~Model()
	{
		int i;
		for ( i = 0; i < m_numMeshes; i++ )
			delete[] m_pMeshes[i].m_pTriangleIndices;
		for ( i = 0; i < m_numMaterials; i++ )
			delete[] m_pMaterials[i].m_pTextureFilename;

		m_numMeshes = 0;
		if ( m_pMeshes != NULL )
		{
			delete[] m_pMeshes;
			m_pMeshes = NULL;
		}

		m_numMaterials = 0;
		if ( m_pMaterials != NULL )
		{
			delete[] m_pMaterials;
			m_pMaterials = NULL;
		}

		m_numTriangles = 0;
		if ( m_pTriangles != NULL )
		{
			delete[] m_pTriangles;
			m_pTriangles = NULL;
		}

		m_numVertices = 0;
		if ( m_pVertices != NULL )
		{
			delete[] m_pVertices;
			m_pVertices = NULL;
		}
	}

	void Model::draw() 
	{
		GLboolean texEnabled = glIsEnabled( GL_TEXTURE_2D );

		// Draw by group
		for ( int i = 0; i < m_numMeshes; i++ )
		{
			int materialIndex = m_pMeshes[i].m_materialIndex;
			if(materialIndex >= 0){
				glMaterialfv( GL_FRONT, GL_AMBIENT, m_pMaterials[materialIndex].m_ambient );
				glMaterialfv( GL_FRONT, GL_DIFFUSE, m_pMaterials[materialIndex].m_diffuse );
				glMaterialfv( GL_FRONT, GL_SPECULAR, m_pMaterials[materialIndex].m_specular );
				glMaterialfv( GL_FRONT, GL_EMISSION, m_pMaterials[materialIndex].m_emissive );
				glMaterialf( GL_FRONT, GL_SHININESS, m_pMaterials[materialIndex].m_shininess );

				if(m_pMaterials[materialIndex].m_texture > 0){
					glEnable( GL_TEXTURE_2D );
					glBindTexture( GL_TEXTURE_2D, m_pMaterials[materialIndex].m_texture );
				}else{
					glDisable( GL_TEXTURE_2D );
				}
			}else{
				glDisable(GL_TEXTURE_2D);
			}

			glBegin(GL_TRIANGLES);
			{
				for(int j = 0; j < m_pMeshes[i].m_numTriangles; j++)
				{
					int triangleIndex = m_pMeshes[i].m_pTriangleIndices[j];
					const Triangle* pTri = &m_pTriangles[triangleIndex];

					for(int k = 0; k < 3; k++)
					{
						int index = pTri->m_vertexIndices[k];

						glNormal3fv( pTri->m_vertexNormals[k] );
						glTexCoord2f( pTri->m_s[k], pTri->m_t[k] );
						glVertex3fv( m_pVertices[index].m_location );
					}
				}
			}
			glEnd();
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		if(texEnabled){
			glEnable(GL_TEXTURE_2D);
		}else{
			glDisable(GL_TEXTURE_2D);
		}
	}

	void Model::reloadTextures(KLGL &gc)
	{
		for ( int i = 0; i < m_numMaterials; i++ ){
			if ( strlen( m_pMaterials[i].m_pTextureFilename ) > 0 ){
				printf("+ %s ", m_pMaterials[i].m_pTextureFilename);
				KLGLTexture* texture = new KLGLTexture(m_pMaterials[i].m_pTextureFilename);
				m_pMaterials[i].m_texture = texture->gltexture;
			}else{
				m_pMaterials[i].m_texture = 0;
			}
		}
	}

	MilkshapeModel::MilkshapeModel()
	{
	}

	MilkshapeModel::~MilkshapeModel()
	{
	}

	/* 
	MS3D STRUCTURES 
	*/

	// byte-align structures
#ifdef _MSC_VER
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error you must byte-align these structures with the appropriate compiler directives
#endif

	typedef unsigned char byte;
	typedef unsigned short word;

	// File header
	struct MS3DHeader
	{
		char m_ID[10];
		int m_version;
	} PACK_STRUCT;

	// Vertex information
	struct MS3DVertex
	{
		byte m_flags;
		float m_vertex[3];
		char m_boneID;
		byte m_refCount;
	} PACK_STRUCT;

	// Triangle information
	struct MS3DTriangle
	{
		word m_flags;
		word m_vertexIndices[3];
		float m_vertexNormals[3][3];
		float m_s[3], m_t[3];
		byte m_smoothingGroup;
		byte m_groupIndex;
	} PACK_STRUCT;

	// Material information
	struct MS3DMaterial
	{
		char m_name[32];
		float m_ambient[4];
		float m_diffuse[4];
		float m_specular[4];
		float m_emissive[4];
		float m_shininess;	// 0.0f - 128.0f
		float m_transparency;	// 0.0f - 1.0f
		byte m_mode;	// 0, 1, 2 is unused now
		char m_texture[128];
		char m_alphamap[128];
	} PACK_STRUCT;

	//	Joint information
	struct MS3DJoint
	{
		byte m_flags;
		char m_name[32];
		char m_parentName[32];
		float m_rotation[3];
		float m_translation[3];
		word m_numRotationKeyframes;
		word m_numTranslationKeyframes;
	} PACK_STRUCT;

	// Keyframe data
	struct MS3DKeyframe
	{
		float m_time;
		float m_parameter[3];
	} PACK_STRUCT;

	// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT

	int MilkshapeModel::loadModelData(KLGL &gc, const char *filename)
	{
		using namespace std;

		printf("Loading Model: %s ", filename);
		ifstream inputFile(filename, ios::in | ios::binary);
		if(inputFile.fail()){
			printf("[ERROR] - Couldn't open the model file.\n");
			return 1; // "Couldn't open the model file."
		}

		inputFile.seekg( 0, ios::end );
		long fileSize = inputFile.tellg();
		inputFile.seekg( 0, ios::beg );

		byte *pBuffer = new byte[fileSize];
		inputFile.read((char*)pBuffer, fileSize);
		inputFile.close();

		const byte *pPtr = pBuffer;
		MS3DHeader *pHeader = ( MS3DHeader* )pPtr;
		pPtr += sizeof( MS3DHeader );

		if(strncmp( pHeader->m_ID, "MS3D000000", 10 ) != 0){
			printf("[ERROR] - Not a valid Milkshape3D model file.\n");
			return 1; // "Not a valid Milkshape3D model file."
		}
		if ( pHeader->m_version < 3 || pHeader->m_version > 4 ){
			printf("[ERROR] - Unhandled file version. Only Milkshape3D Version 1.3 and 1.4 is supported.\n");
			return 1; // "Unhandled file version. Only Milkshape3D Version 1.3 and 1.4 is supported." );
		}
		int nVertices = *( word* )pPtr; 
		m_numVertices = nVertices;
		m_pVertices = new Vertex[nVertices];
		pPtr += sizeof( word );

		int i;
		for ( i = 0; i < nVertices; i++ )
		{
			MS3DVertex *pVertex = ( MS3DVertex* )pPtr;
			m_pVertices[i].m_boneID = pVertex->m_boneID;
			memcpy( m_pVertices[i].m_location, pVertex->m_vertex, sizeof( float )*3 );
			pPtr += sizeof( MS3DVertex );
		}

		int nTriangles = *( word* )pPtr;
		m_numTriangles = nTriangles;
		m_pTriangles = new Triangle[nTriangles];
		pPtr += sizeof( word );

		for ( i = 0; i < nTriangles; i++ )
		{
			MS3DTriangle *pTriangle = ( MS3DTriangle* )pPtr;
			int vertexIndices[3] = { pTriangle->m_vertexIndices[0], pTriangle->m_vertexIndices[1], pTriangle->m_vertexIndices[2] };
			float t[3] = { 1.0f-pTriangle->m_t[0], 1.0f-pTriangle->m_t[1], 1.0f-pTriangle->m_t[2] };
			memcpy( m_pTriangles[i].m_vertexNormals, pTriangle->m_vertexNormals, sizeof( float )*3*3 );
			memcpy( m_pTriangles[i].m_s, pTriangle->m_s, sizeof( float )*3 );
			memcpy( m_pTriangles[i].m_t, t, sizeof( float )*3 );
			memcpy( m_pTriangles[i].m_vertexIndices, vertexIndices, sizeof( int )*3 );
			pPtr += sizeof( MS3DTriangle );
		}

		int nGroups = *( word* )pPtr;
		m_numMeshes = nGroups;
		m_pMeshes = new Mesh[nGroups];
		pPtr += sizeof( word );
		for ( i = 0; i < nGroups; i++ )
		{
			pPtr += sizeof( byte );	// flags
			pPtr += 32;				// name

			word nTriangles = *( word* )pPtr;
			pPtr += sizeof( word );
			int *pTriangleIndices = new int[nTriangles];
			for ( int j = 0; j < nTriangles; j++ )
			{
				pTriangleIndices[j] = *( word* )pPtr;
				pPtr += sizeof( word );
			}

			char materialIndex = *( char* )pPtr;
			pPtr += sizeof( char );

			m_pMeshes[i].m_materialIndex = materialIndex;
			m_pMeshes[i].m_numTriangles = nTriangles;
			m_pMeshes[i].m_pTriangleIndices = pTriangleIndices;
		}

		int nMaterials = *( word* )pPtr;
		m_numMaterials = nMaterials;
		m_pMaterials = new Material[nMaterials];
		pPtr += sizeof( word );
		for ( i = 0; i < nMaterials; i++ )
		{
			MS3DMaterial *pMaterial = ( MS3DMaterial* )pPtr;
			memcpy( m_pMaterials[i].m_ambient, pMaterial->m_ambient, sizeof( float )*4 );
			memcpy( m_pMaterials[i].m_diffuse, pMaterial->m_diffuse, sizeof( float )*4 );
			memcpy( m_pMaterials[i].m_specular, pMaterial->m_specular, sizeof( float )*4 );
			memcpy( m_pMaterials[i].m_emissive, pMaterial->m_emissive, sizeof( float )*4 );
			m_pMaterials[i].m_shininess = pMaterial->m_shininess;
			m_pMaterials[i].m_pTextureFilename = new char[strlen( pMaterial->m_texture )+1];
			strcpy( m_pMaterials[i].m_pTextureFilename, pMaterial->m_texture );
			pPtr += sizeof( MS3DMaterial );
		}

		reloadTextures(gc);
		delete[] pBuffer;

		printf("[OK]\n");
		return 0;
	}
}