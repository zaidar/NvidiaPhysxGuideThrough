#include <ctype.h>

#include "PxPhysicsAPI.h"
#include"../../physx/snippets/snippetcommon/SnippetPrint.h"
#include"../../physx/snippets/snippetcommon/SnippetPVD.h"
#include"../../physx/snippets/snippetutils/SnippetUtils.h"

#include "cooking/PxCooking.h"
#include "cooking/PxConvexMeshDesc.h"
#include "geometry/PxConvexMeshGeometry.h"

using namespace physx;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;

PxFoundation*			gFoundation = NULL;
PxPhysics*				gPhysics = NULL;      // Combines foundation and pivot
											  // Creates objects 

PxDefaultCpuDispatcher*	gDispatcher = NULL;
PxScene*				gScene = NULL;

PxMaterial*				gMaterial = NULL;

PxPvd*                  gPvd = NULL;

PxReal stackZ = 10.0f;

// Ball with velocity
PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0))
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}

void initPhysics()
{
	// Ground memory & log
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	// Scene 
	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, 
							   PxTolerancesScale(), true, gPvd);
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());

	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(3);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
	// Ground
	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);

	// Cubes 0
	/*
	PxReal halfExtent = 1.0f;
	PxRigidDynamic* body;
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);

	auto cube = [&body, shape](PxU32 i, PxU32 j) {
		PxTransform localTm(PxVec3(PxReal(j), PxReal(.1), PxReal(i)));
		body = gPhysics->createRigidDynamic(localTm);
		body->attachShape(*shape);
		gScene->addActor(*body);
		shape->release();

	};
	
	int countX = 0, countY=0;
	PxU32 size = 81;
	
	for (PxU32 i = 1; i < size * 4 + 1; i += 4) {
		for (PxU32 j = 1; j < size * 4 ; j += 4)
			cube(i, j);
		
		countX = 0; countY += 4;
	}
	cube(0, 0);
	

	// Exclusive shapes 1/
	/*PxReal halfHeight= 2.f, radius= .1f;
	PxRigidDynamic* aCapsuleActor = gPhysics->createRigidDynamic(PxTransform(PxVec3(0, 0, 40)));
	PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
	static PxShape* aCapsuleShape = PxRigidActorExt::createExclusiveShape(*aCapsuleActor, PxCapsuleGeometry(radius, halfHeight), *gMaterial);
	aCapsuleShape->setLocalPose(relativePose);//   PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial
	PxRigidBodyExt::updateMassAndInertia(*aCapsuleActor, 50);
	gScene->addActor(*aCapsuleActor);
	*/

	// Convex shapes 2/
	
	/*static const PxVec3 convexVerts[] = { PxVec3(0,41,0),PxVec3(41,0,0),PxVec3(-1,0,0),PxVec3(0,0,41),
	PxVec3(0,0,-1) };
	PxRigidDynamic* aCapsuleActor = gPhysics->createRigidDynamic(PxTransform(PxVec3(0, 0, 40)));
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = 5;
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = convexVerts;
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxCooking *cooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(gPhysics->getTolerancesScale()));
	PxConvexMeshCookingResult::Enum result;
	PxDefaultMemoryOutputStream buf;
	if (!cooking->cookConvexMesh(convexDesc, buf, &result)) 
		return;
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = gPhysics->createConvexMesh(input);

	PxShape* aConvexShape = PxRigidActorExt::createExclusiveShape(*aCapsuleActor, PxConvexMeshGeometry(convexMesh), *gMaterial);
	PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0, 0, 1)));

	aConvexShape->setLocalPose(relativePose);
	PxRigidBodyExt::updateMassAndInertia(*aCapsuleActor, 50);
	gScene->addActor(*aCapsuleActor);*/
	



	/*//v2
	static const PxVec3 convexVerts[] = { 
		PxVec3(0,41,0), PxVec3(41,0,0), PxVec3(41,0,0)
		,PxVec3(0,0,10),
	};

	PxRigidDynamic* aCapsuleActor = gPhysics->createRigidDynamic(PxTransform(PxVec3(0, 0, 0)));
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = 5;
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = convexVerts;
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxCooking *cooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(gPhysics->getTolerancesScale()));
	PxConvexMeshCookingResult::Enum result;
	PxDefaultMemoryOutputStream buf;
	if (!cooking->cookConvexMesh(convexDesc, buf, &result))
		return;
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = gPhysics->createConvexMesh(input);

	PxShape* aConvexShape = PxRigidActorExt::createExclusiveShape(*aCapsuleActor, PxConvexMeshGeometry(convexMesh), *gMaterial);
	PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0, 0, 1)));

	aConvexShape->setLocalPose(relativePose);
	//PxRigidBodyExt::updateMassAndInertia(*aCapsuleActor, 50);
	//gScene->addActor(*aCapsuleActor);
	*/
	static const PxVec3 verts[] = { PxVec3(0,100,0), PxVec3(100,0,0), PxVec3(0,0,100) };



	PxRigidDynamic* aCapsuleActor = gPhysics->createRigidDynamic(PxTransform(PxVec3(0, 0, 0)));
	PxTriangleMeshDesc meshDesc;
	static const PxU16 indices[] = { 0, 1 ,2 };
	meshDesc.points.count = 3;//numVertices;
	meshDesc.points.data = verts;
	meshDesc.points.stride = sizeof(PxVec3);
	meshDesc.triangles.count = 1;//numTriangles;
	meshDesc.triangles.data = indices;
	meshDesc.triangles.stride = 3 * sizeof(PxU16);
	aCapsuleActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	PxCooking *cooking = PxCreateCooking(PX_PHYSICS_VERSION, gPhysics->getFoundation(), PxCookingParams(gPhysics->getTolerancesScale()));
	meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;


	PxTolerancesScale scale;	
	PxCookingParams params = cooking->getParams();

	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;


#ifdef _DEBUG
	bool res = cooking->validateTriangleMesh(meshDesc);
	PX_ASSERT(res);
#endif


	PxTriangleMeshCookingResult::Enum result;
	PxDefaultMemoryOutputStream buf;
	if (!cooking->cookTriangleMesh(meshDesc, buf, &result) )
		return;
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxTriangleMesh* convexMesh = gPhysics->createTriangleMesh(input);

	
	PxShape* aConvexShape;
	//convexMesh = cooking->createTriangleMesh(meshDesc,gPhysics->getPhysicsInsertionCallback());
	
	aConvexShape = PxRigidActorExt::createExclusiveShape(*aCapsuleActor, PxTriangleMeshGeometry(convexMesh), *gMaterial);
	//aConvexShape->setFlags(PxShapeFlag::eSIMULATION_SHAPE);

	gScene->addActor(*aCapsuleActor);
	

}
void stepPhysics(bool /*interactive*/)
{
	gScene->simulate(1.0f / 60.0f);
	gScene->fetchResults(true);
}
// Free memory
void cleanupPhysics(bool /*interactive*/)
{
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	if (gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();	gPvd = NULL;
		PX_RELEASE(transport);
	}
	PX_RELEASE(gFoundation);
}
// Key handle
void keyPress(unsigned char key, const PxTransform& camera)
{
	switch (toupper(key))
	{
	case 'F':	createDynamic(camera, PxSphereGeometry(3.0f), camera.rotate(PxVec3(0, 0, -1)) * 100);		break;
	}
}

int main(const int ,const char** )
{
	extern void renderLoop();
	renderLoop();

	return 0;
}
