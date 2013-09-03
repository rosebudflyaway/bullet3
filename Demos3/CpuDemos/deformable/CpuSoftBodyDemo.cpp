#include "CpuSoftBodyDemo.h"
#include "OpenGLWindow/ShapeData.h"
#include "OpenGLWindow/GLInstancingRenderer.h"
#include "Bullet3Common/b3Quaternion.h"
#include "OpenGLWindow/b3gWindowInterface.h"


#include "OpenGLWindow/OpenGLInclude.h"
#include "OpenGLWindow/GLInstanceRendererInternalData.h"

#include "Bullet3Collision/NarrowPhaseCollision/b3Config.h"
#include "CpuSoftBodyDemoInternalData.h"
#include "Bullet3Collision/BroadPhaseCollision/b3DynamicBvhBroadphase.h"
#include "stb_image/stb_image.h"
#include "CpuSoftClothDemoInternalData.h"

static b3KeyboardCallback oldCallback = 0;
extern bool gReset;

float clothWidth = 4;
float clothHeight= 4;

int width = 64;
int height = 64;
int numPoints = width*height;
float clothMass = 100.f;



CpuSoftBodyDemo::CpuSoftBodyDemo()
:m_instancingRenderer(0),
m_window(0)
{
	m_data = new CpuSoftBodyDemoInternalData;
}
CpuSoftBodyDemo::~CpuSoftBodyDemo()
{
	
	delete m_data;
}







static void PairKeyboardCallback(int key, int state)
{
	if (key=='R' && state)
	{
		gReset = true;
	}
	
	//b3DefaultKeyboardCallback(key,state);
	oldCallback(key,state);
}

void	CpuSoftBodyDemo::setupScene(const ConstructionInfo& ci)
{

}

void	CpuSoftBodyDemo::initPhysics(const ConstructionInfo& ci)
{

	GLint err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	if (ci.m_window)
	{
		m_window = ci.m_window;
		oldCallback = ci.m_window->getKeyboardCallback();
		ci.m_window->setKeyboardCallback(PairKeyboardCallback);

	}

	m_instancingRenderer = ci.m_instancingRenderer;

	

	setupScene(ci);

	err = glGetError();
	b3Assert(err==GL_NO_ERROR);



	m_instancingRenderer->writeTransforms();
	
}

void	CpuSoftBodyDemo::exitPhysics()
{

	m_window->setKeyboardCallback(oldCallback);
	
}

	
#include "Bullet3Common/shared/b3Float4.h"

struct GraphicsVertex
{
	float pos[4];
	float normal[3];
	float texcoord[2];
};

void CpuSoftClothDemo::computeForces()
{
	B3_PROFILE("computeForces");

	b3Vector3 gravityAcceleration = b3MakeVector3(0,-9.8,0);
	//f=m*a
	for (int i=0;i<numPoints;i++)
	{
		{
			float	particleMass = m_clothData->m_particleMasses[i];

			b3Vector3 particleMassVec = b3MakeVector3(particleMass,particleMass,particleMass,0);
			m_clothData->m_forces[i] = gravityAcceleration*particleMass;
		}
	}

	GraphicsVertex* cpu_buffer = (GraphicsVertex*)m_data->m_clothVertices;


	//add spring forces
	for(int i=0;i<m_clothData->m_springs.size();i++) 
	{

		int indexA = m_clothData->m_springs[i].m_particleIndexA;
		int indexB = m_clothData->m_springs[i].m_particleIndexB;
		float restLength = m_clothData->m_springs[i].m_restLength;
		const ClothMaterial& mat = m_clothData->m_materials[m_clothData->m_springs[i].m_material];

		const b3Vector3& posA = (const b3Vector3&)cpu_buffer[indexA].pos;
		const b3Vector3& posB = (const b3Vector3&)cpu_buffer[indexB].pos;
		const b3Vector3& velA = m_clothData->m_velocities[indexA];
		const b3Vector3& velB = m_clothData->m_velocities[indexB];

		b3Vector3 deltaP = posA-posB;
		b3Vector3 deltaV = velA-velB;
		float dist = deltaP.length();
		b3Vector3 deltaPNormalized = deltaP/dist;

		float spring = -mat.m_stiffness * (dist-restLength)*100000;
		float damper = mat.m_damping * b3Dot(deltaV,deltaPNormalized)*100;

		b3Vector3 springForce = (spring+damper)*deltaPNormalized;
		float particleMassA = m_clothData->m_particleMasses[indexA];
		float particleMassB = m_clothData->m_particleMasses[indexB];

		//if (springForce.length())
		{
			if (particleMassA)
			{
				m_clothData->m_forces[indexA] += springForce*particleMassA;
			}

			if (particleMassB)
			{
				m_clothData->m_forces[indexB] -= springForce*particleMassB;
			}
		}
	}
}

void CpuSoftClothDemo::integrateEuler(float deltaTime)
{
	B3_PROFILE("integrateEuler");
	b3Vector3 deltaTimeVec = b3MakeVector3(deltaTime,deltaTime,deltaTime,0);

	GraphicsVertex* cpu_buffer = (GraphicsVertex*)m_data->m_clothVertices;

	for (int i=0;i<numPoints;i++)
	{
		float mass = m_clothData->m_particleMasses[i];
		if (mass)
		{
			


			b3Vector3 dv = (m_clothData->m_forces[i]/mass)*deltaTimeVec;
			m_clothData->m_velocities[i]+= dv;
			m_clothData->m_velocities[i]*=0.999;

			b3Vector3& pos = (b3Vector3&) cpu_buffer[i].pos;
			pos += m_clothData->m_velocities[i]*deltaTimeVec;
		}
	}
}

void CpuSoftClothDemo::renderScene()
{
	m_instancingRenderer->renderScene();

}
void CpuSoftBodyDemo::renderScene()
{
	m_instancingRenderer->renderScene();
}

void CpuSoftBodyDemo::clientMoveAndDisplay()
{
	GLint err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	

}


void CpuSoftClothDemo::clientMoveAndDisplay()
{
	if (m_data->m_clothShapeIndex>=0 && m_data->m_clothVertices)
	{
		
		float deltaTime = 1./1000.;//1./60.f;
		//float deltaTime = 1./60.f;
		for (int i=0;i<10;i++)
		{
			computeForces();
			integrateEuler(deltaTime);
		
		}
		m_instancingRenderer->updateShape(m_data->m_clothShapeIndex,m_data->m_clothVertices);
	}
}

CpuSoftClothDemo::CpuSoftClothDemo()
{
	m_clothData = new CpuSoftClothDemoInternalData();

}
CpuSoftClothDemo::~CpuSoftClothDemo()
{
	delete m_clothData;
}
	

unsigned char* CpuSoftClothDemo::loadImage(const char* fileName, int& width, int& height, int& n)
{
		unsigned char *data = stbi_load(fileName, &width, &height, &n, 0);
		if (data)
		{
			GLubyte*	image=new GLubyte[512*256*4];
			for(int y=0;y<256;++y)
			{
				const int	t=y>>4;
				GLubyte*	pi=image+y*512*3;
				for(int x=0;x<512;++x)
				{
					const int		s=x>>5;
					const GLubyte	b=180;					
					GLubyte			c=b+((s+t&1)&1)*(255-b);
					pi[0]=pi[1]=pi[2]=c;pi+=3;
				}
			}

			{
			
				for (int i=0;i<width;i++)
				{
					for (int j=0;j<height;j++)
					{
						int offsetx = (512-width)/2;
						int offsety = (256-height)/2;

						GLubyte*	pi=image+((j+offsety)*512+i+offsetx)*3;
						const GLubyte*	src=data+(j*width+i)*4;
						pi[0] = src[0];
						pi[1] = src[1];
						pi[2] = src[2];
					}
				}

			
			}
			width = 512;
			height = 256;
			return image;
		}
		return 0;
}

void	CpuSoftClothDemo::setupScene(const ConstructionInfo& ci)
{
	GLint err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	

	GraphicsVertex* cpu_buffer = new GraphicsVertex[width*height];
	memset(cpu_buffer, 0, width*height*sizeof(GraphicsVertex));


	int numVertices = 0;
	// Initial test data for rendering
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			double coord = b3Sin(x/5.0)*0.01+1;
			//coord = sin(y/);

			float posX = (x/((float)(width-1)))*(clothWidth);
			float posZ = ((y-height/2.f)/((float)(height-1)))*(clothHeight);

			cpu_buffer[y*width+x].pos[0]      = posX;
			cpu_buffer[y*width+x].pos[1]      = coord;
			cpu_buffer[y*width+x].pos[2]      = posZ; 
			cpu_buffer[y*width+x].pos[3]      = 0.f;

			cpu_buffer[y*width+x].normal[0]   = 1;
			cpu_buffer[y*width+x].normal[1]   = 0;
			cpu_buffer[y*width+x].normal[2]   = 0;
			cpu_buffer[y*width+x].texcoord[0] = 1*x/((float)(width-1));
			cpu_buffer[y*width+x].texcoord[1] = (1.f-4*y/((float)(height-1)));
			numVertices++;
		}
	}

	err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	int numIndices = 0;

	// Generate and fill index array for rendering
	 int* indices = new  int[width*3*2+2 + height*width*3*2];

	for(int y = 0; y < height-1; y++)
	{
		for(int x = 0; x < width-1; x++)
		{
			// *3 indices/triangle, *2 triangles/quad
			int baseIndex = (x + y*(width-1))*3*2;
			indices[baseIndex] = x + y*width;
			indices[baseIndex+1] = x+1 + y*width;
			indices[baseIndex+2] = x+width + y*width;


			indices[baseIndex+3] = x + 1 +  y*width;
			indices[baseIndex+4] = x+(width+1) + y*width;
			indices[baseIndex+5] = x+width + y*width;
			numIndices+=6;

		}
	}


	int num_barrel_vertices = sizeof(barrel_vertices)/(9*sizeof(float));
	int num_barrel_indices = sizeof(barrel_indices)/sizeof(int);

	int textureIndex  = -1;
	{
		int width,height,n;
		FILE* f = fopen("test.tst","wb");
		fclose(f);
		const char* filename = "data/bullet_logo.png";
		const unsigned char* image=0;
		
		const char* prefix[]={"./","../","../../","../../../","../../../../"};
		int numprefix = sizeof(prefix)/sizeof(const char*);
		
		for (int i=0;!image && i<numprefix;i++)
		{
			char relativeFileName[1024];
			sprintf(relativeFileName,"%s%s",prefix[i],filename);
			image = loadImage(relativeFileName,width,height,n);
		}
		
		b3Assert(image);
		if (image)
		{
			textureIndex = ci.m_instancingRenderer->registerTexture(image,width,height);
		}
	}
//	int shapeIndex = ci.m_instancingRenderer->registerShape(barrel_vertices,num_barrel_vertices,barrel_indices,num_barrel_indices);

	err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	m_data->m_clothVertices = &cpu_buffer[0].pos[0];

	int shapeIndex = ci.m_instancingRenderer->registerShape(&cpu_buffer[0].pos[0],numVertices,indices,numIndices,B3_GL_TRIANGLES,textureIndex);
	m_data->m_clothShapeIndex = shapeIndex;

	err = glGetError();
	b3Assert(err==GL_NO_ERROR);

	
	float pos[4] = {0,0,0,0};
	float orn[4] = {0,0,0,1};
	float color[4] = {1,1,1,1};
	float scaling[4] = {10,10,10,1};

	ci.m_instancingRenderer->registerGraphicsInstance(shapeIndex,pos,orn,color,scaling);
	ci.m_instancingRenderer->setCameraDistance(24);
	ci.m_instancingRenderer->setCameraTargetPosition(pos);

	err = glGetError();
	b3Assert(err==GL_NO_ERROR);


	int numParticles = width*height;

	m_clothData->m_forces.resize(numParticles);
	m_clothData->m_velocities.resize(numParticles);
	m_clothData->m_particleMasses.resize(numParticles);

	

	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			float  mass = clothMass/numParticles;
			if (x==0 && y==(height-1))
				mass=0.f;
			if (x==0 && y==0)
				mass=0.f;
			int index = x+width*y;

			m_clothData->m_particleMasses[index] = mass;
		}
	}

	
	ClothMaterial mat;
	mat.m_stiffness = 0.5;
	mat.m_damping = -0.25;

	m_clothData->m_materials.push_back(mat);


	//add springs
	for (int x=0;x<width-1;x++)
	{
		for (int y=0;y<height;y++)
		{
			ClothSpring spring;
			int indexA = x+y*width;
			int indexB = x+1+y*width;
			spring.m_particleIndexA = indexA;
			spring.m_particleIndexB = indexB;
			spring.m_material = 0;
			const b3Vector3& posA = (const b3Vector3&) cpu_buffer[indexA].pos;
			const b3Vector3& posB = (const b3Vector3&) cpu_buffer[indexB].pos;
			spring.m_restLength = (posA-posB).length();
			m_clothData->m_springs.push_back(spring);

		}
	}

	for (int x=0;x<width;x++)
	{
		for (int y=0;y<height-1;y++)
		{
			ClothSpring spring;
			int indexA = x+y*width;
			int indexB = x+(y+1)*width;
			spring.m_particleIndexA = indexA;
			spring.m_particleIndexB = indexB;
			spring.m_material = 0;
			const b3Vector3& posA = (const b3Vector3&) cpu_buffer[indexA].pos;
			const b3Vector3& posB = (const b3Vector3&) cpu_buffer[indexB].pos;
			spring.m_restLength = (posA-posB).length();
			m_clothData->m_springs.push_back(spring);
		}
	}

	for (int x=0;x<width-1;x++)
	{
		for (int y=0;y<height-1;y++)
		{
			{
				ClothSpring spring;
				int indexA = x+y*width;
				int indexB = (x+1)+(y+1)*width;
				spring.m_particleIndexA = indexA;
				spring.m_particleIndexB = indexB;
				spring.m_material = 0;
				const b3Vector3& posA = (const b3Vector3&) cpu_buffer[indexA].pos;
				const b3Vector3& posB = (const b3Vector3&) cpu_buffer[indexB].pos;
				spring.m_restLength = (posA-posB).length();
				m_clothData->m_springs.push_back(spring);
			}

			{
				ClothSpring spring;
				int indexA = x+1+y*width;
				int indexB = (x)+(y+1)*width;
				spring.m_particleIndexA = indexA;
				spring.m_particleIndexB = indexB;
				spring.m_material = 0;
				const b3Vector3& posA = (const b3Vector3&) cpu_buffer[indexA].pos;
				const b3Vector3& posB = (const b3Vector3&) cpu_buffer[indexB].pos;
				spring.m_restLength = (posA-posB).length();
				m_clothData->m_springs.push_back(spring);
			}

		}
	}


}	