#include <OGRE/OgreRoot.h>
#include <OGRE/OgreRenderSystem.h>
#include <OGRE/OgreRenderWindow.h>
#include <OGRE/OgreWindowEventUtilities.h>

#include <iostream>
#include <memory>
#include <exception>

//#include <OIS/Ois.h>

#include "SinbadCharacterController.h"
#include <btBulletDynamicsCommon.h>


bool continue_looping = true;

SinbadCharacterController* mChara = NULL;

class InputHandler : public OIS::KeyListener , public OIS::MouseListener
{
public:
    bool keyPressed(const OIS::KeyEvent& evt)
    {
        if(evt.key == OIS::KC_ESCAPE)
            continue_looping = false;
        mChara->injectKeyDown(evt);
        return true;
    }
    
    bool keyReleased(const OIS::KeyEvent& evt)
    {
        mChara->injectKeyUp(evt);
        return true;
    }
    
    bool mouseMoved(const OIS::MouseEvent& evt)
	{
		// relay input events to character controller
		mChara->injectMouseMove(evt);
		return true;
	}

	bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		// relay input events to character controller
		mChara->injectMouseDown(evt, id);
		return true;
	}
	
	bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		return true;
	}
};

int main()
{
    Ogre::String lConfigFileName = "ogre.cfg";
    Ogre::String lPluginsFileName = "plugins.cfg";
    Ogre::String lLogFileName = "Tutorial_Ogre.log";
    
    std::auto_ptr<Ogre::Root> lRoot(
        new Ogre::Root(lConfigFileName,
        lPluginsFileName, lLogFileName));
    
    typedef std::vector<Ogre::String> Strings;
    
    
    //lRoot->loadPlugins(lPluginsFileName);
    lRoot->loadPlugin("/usr/lib/i386-linux-gnu/OGRE-1.9.0/RenderSystem_GL");
    //lRoot->showConfigDialog();
    
    /*Ogre::String pluginFolder = "/usr/lib/OGRE/";
    Strings lPluginNames;
    lPluginNames.push_back(pluginFolder + "RenderSystem_GL");
    lPluginNames.push_back(pluginFolder + "Plugin_ParticleFX");
    lPluginNames.push_back(pluginFolder + "Plugin_CgProgramManager");
    lPluginNames.push_back(pluginFolder + "Plugin_OctreeSceneManager");
    
    {
        Strings::iterator lIter = lPluginNames.begin();
        Strings::iterator lIterEnd = lPluginNames.end();
        
        for (;lIter != lIterEnd; lIter++)
        {
            Ogre::String& lPluginName = (*lIter);
            
            lRoot->loadPlugin(lPluginName);
        }
    }*/
    
    const Ogre::RenderSystemList& lRenderSystemList = lRoot->getAvailableRenderers();
    if( lRenderSystemList.size() == 0 )
    {
        Ogre::LogManager::getSingleton().logMessage("Sorry, no rendersystem was found.");
        return 1;
    }
    
    Ogre::RenderSystem* lRenderSystem = lRenderSystemList[0];
    lRoot->setRenderSystem(lRenderSystem);
    
    bool lCreateAWindowAutomatically = false;
    Ogre::String lWindowTitle = "";
    Ogre::String lCustomCapacities = "";
    
    lRoot->initialise(lCreateAWindowAutomatically, lWindowTitle, lCustomCapacities);
    
    Ogre::RenderWindow* lWindow = NULL;
    {
        Ogre::String lWindowTitle = "Hello Ogre World";
        
        unsigned int lSizeX = 640;
        unsigned int lSizeY = 480;
        
        bool lFullscreen = false;
        
        Ogre::NameValuePairList lParams;
        
        lParams["FSAA"] = "0";
        
        lParams["vsync"] = "true";
        lWindow = lRoot->createRenderWindow(lWindowTitle, lSizeX, lSizeY, lFullscreen, &lParams);
    }
    
    InputHandler mInputHandler;
    OIS::InputManager* mInputManager = NULL;
    OIS::Keyboard* mKeyboard = NULL;
    OIS::Mouse* mMouse = NULL;
    {
        size_t windowHandle = 0;
        lWindow->getCustomAttribute("WINDOW", &windowHandle);
        
        std::string windowsHandleAsString="";
        {
            std::ostringstream windowHndStr;
            windowHndStr << windowHandle;
            windowsHandleAsString = windowHndStr.str();
        }
        
        OIS::ParamList lSpecialParameters;
        lSpecialParameters.insert(std::make_pair(std::string("WINDOW"),
            windowsHandleAsString));
            
            mInputManager = OIS::InputManager::createInputSystem(lSpecialParameters);
            
            mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(
                OIS::OISKeyboard, true));
                
            if (mInputManager->getNumberOfDevices(OIS::OISMouse) > 0)
            {
                mMouse = static_cast<OIS::Mouse*>( mInputManager->createInputObject(
                OIS::OISMouse, true ) );
            }
    }
    
    //mKeyboard->setEventCallback(&mInputHandler);
    
    Ogre::Viewport* mViewport = NULL;
    Ogre::SceneManager* mSceneMgr = NULL;
    Ogre::Camera* mCamera = NULL;
    //SdkTrayManager* mTrayManager;
    
    ResourceGroupManager::getSingleton().addResourceLocation(
            "resource", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "Sinbad.zip", "Zip", "Sinbad");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "materials/programs", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "materials/scripts", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "materials/textures", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "data/Textures", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            "data/Meshes", "FileSystem", "General");
    ResourceGroupManager::getSingleton().addResourceLocation(
            ".", "FileSystem", "General");
    
    
    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    {
        mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);
	    // setup default viewport layout and camera
		mCamera = mSceneMgr->createCamera("MainCamera");
		mViewport = lWindow->addViewport(mCamera);
		mCamera->setAspectRatio((Ogre::Real)mViewport->getActualWidth() / (Ogre::Real)mViewport->getActualHeight());
		mCamera->setNearClipDistance(5);
		
		//mCamera->move(Ogre::Vector3(0,5,0));

		//mCameraMan = new SdkCameraMan(mCamera);   // create a default camera controller
	}
    
    {   
		// set background and some fog
		mViewport->setBackgroundColour(ColourValue(1.0f, 1.0f, 0.8f));
		mSceneMgr->setFog(Ogre::FOG_LINEAR, ColourValue(1.0f, 1.0f, 0.8f), 0, 20, 100);

		// set shadow properties
		/*mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
		mSceneMgr->setShadowColour(ColourValue(0.5, 0.5, 0.5));
		mSceneMgr->setShadowTextureSize(1024);
		mSceneMgr->setShadowTextureCount(1);*/

		// disable default camera control so the character can do its own
		//mCameraMan->setStyle(CS_MANUAL);

		// use a small amount of ambient lighting
		mSceneMgr->setAmbientLight(ColourValue(1, 1, 1));

		// add a bright light above the scene
		/*Light* light = mSceneMgr->createLight();
		light->setType(Light::LT_POINT);
		light->setPosition(-10, 40, 20);
		light->setSpecularColour(ColourValue::White);*/

		// create a floor mesh resource
		/*MeshManager::getSingleton().createPlane("floor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Plane(Vector3::UNIT_Y, 0), 100, 100, 10, 10, true, 1, 10, 10, Vector3::UNIT_Z);

		// create a floor entity, give it a material, and place it at the origin
        Entity* floor = mSceneMgr->createEntity("Floor", "floor");
        floor->setMaterialName("Examples/Rockwall");
		floor->setCastShadows(false);
        mSceneMgr->getRootSceneNode()->attachObject(floor);*/

//		LogManager::getSingleton().logMessage("toggling stats");
		//mTrayMgr->toggleAdvancedFrameStats();

//		LogManager::getSingleton().logMessage("creating panel");
		//StringVector items;
		//items.push_back("Help");
		//ParamsPanel* help = mTrayMgr->createParamsPanel(TL_TOPLEFT, "HelpMessage", 100, items);
		//help->setParamValue("Help", "H / F1");
		
//		LogManager::getSingleton().logMessage("all done");
	}
        
    //Physics
    btAxisSweep3* mBroadphase = new btAxisSweep3(btVector3(-10000, -10000, -10000), btVector3(10000, 10000, 10000));
	btDefaultCollisionConfiguration* mCollisionConfig = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* mDispatcher = new btCollisionDispatcher(mCollisionConfig);
	btSequentialImpulseConstraintSolver* mSolver = new btSequentialImpulseConstraintSolver();

	btDynamicsWorld* phyWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfig);
	//Globals::phyWorld->setGravity(btVector3(0,-9.8,0));
	phyWorld->getDispatchInfo().m_allowedCcdPenetration=0.0001f;
	
	//----------------------------------------------------------
	// Debug drawing!
	//----------------------------------------------------------

	BtOgre::DebugDrawer* dbgdraw = new BtOgre::DebugDrawer(mSceneMgr->getRootSceneNode(), phyWorld);
	phyWorld->setDebugDrawer(dbgdraw);
	
	//----------------------------------------------------------
    // Kinematic character controller
    //----------------------------------------------------------
        
    btTransform startTransform;
	startTransform.setIdentity ();
	startTransform.setOrigin (btVector3(0.0, 20.0, 0.0));
        
        
	btPairCachingGhostObject* m_ghostObject = new btPairCachingGhostObject();
	m_ghostObject->setWorldTransform(startTransform);
	mBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	btScalar characterHeight=0.8;
	btScalar characterWidth =0.6;
	btConvexShape* capsule = new btCapsuleShape(characterWidth,characterHeight);
	m_ghostObject->setCollisionShape (capsule);
	//m_ghostObject->setCollisionShape (mNinjaShape);
	m_ghostObject->setCollisionFlags (btCollisionObject::CF_CHARACTER_OBJECT);
        
	btScalar stepHeight = btScalar(0.2);
	//m_character = new myKinematicCharacterController (m_ghostObject,capsule,stepHeight);
	btKinematicCharacterController* m_character = new btKinematicCharacterController (m_ghostObject,capsule,stepHeight);
	    
	//Verkar inte göra något
	//EDIT: ändrar maxfallhastigheten efter hopp men inte fall.
	//m_character->setFallSpeed(0.5);
	
	//m_character->setJumpSpeed(20);
	//m_character->setGravity(9.8 * 5);
        
    ///only collide with static for now (no interaction with dynamic objects)
	phyWorld->addCollisionObject(m_ghostObject,btBroadphaseProxy::CharacterFilter,               btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);
	//phyWorld->addCollisionObject(m_ghostObject,btBroadphaseProxy::CharacterFilter);
        
	phyWorld->addAction(m_character);


    m_character->setMaxSlope(btRadians(60.0));    
    
//	LogManager::getSingleton().logMessage("creating sinbad");
	// create our character controller
	mChara = new SinbadCharacterController(mCamera, m_character, phyWorld);
	
	//Set up ground plane
	//btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	
	//btDefaultMotionState* groundMotionState =
    //            new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    
    //btRigidBody::btRigidBodyConstructionInfo
    //            groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
    //    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
        
    //phyWorld->addRigidBody(groundRigidBody);
    
	Ogre::Entity *mGroundEntity;
	btRigidBody *mGroundBody;
	btBvhTriangleMeshShape *mGroundShape;
    
        mGroundEntity = mSceneMgr->createEntity("groundEntity", "Circle.mesh");
        //mGroundEntity = mSceneMgr->createEntity("groundEntity", "data/Meshes/TestLevel_b0.mesh");
	Ogre::SceneNode* groundNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("groundNode");
	groundNode->attachObject(mGroundEntity);
	//groundNode->scale(Ogre::Vector3(3,3,3));

	//Create the ground shape.
	BtOgre::StaticMeshToShapeConverter converter2(mGroundEntity);
	mGroundShape = converter2.createTrimesh();

	//Create MotionState (no need for BtOgre here, you can use it if you want to though).
	btDefaultMotionState* groundState = new btDefaultMotionState(
	   btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)));

	//Create the Body.
	mGroundBody = new btRigidBody(0, groundState, mGroundShape, btVector3(0,0,0));
	phyWorld->addRigidBody(mGroundBody);
    
    //Spikeball
    Ogre::SceneNode *mNinjaNode;
	Ogre::Entity *mNinjaEntity;
	btRigidBody *mNinjaBody;
	btCollisionShape *mNinjaShape;
	
	Vector3 pos = Vector3(2,20,2);
	Quaternion rot = Quaternion::IDENTITY;
	
    mNinjaEntity = mSceneMgr->createEntity("ninjaEntity", "Player.mesh");
	mNinjaNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("ninjaSceneNode", pos, rot);
	mNinjaNode->attachObject(mNinjaEntity);
	
	//mNinjaNode->scale(Vector3(3,3,3));
	
	//Create shape.
	BtOgre::StaticMeshToShapeConverter converter(mNinjaEntity);
	mNinjaShape = converter.createSphere();
	
	//Calculate inertia.
	btScalar mass = 5;
	btVector3 inertia;
	mNinjaShape->calculateLocalInertia(mass, inertia);
	
	//Create BtOgre MotionState (connects Ogre and Bullet).
	BtOgre::RigidBodyState *ninjaState = new BtOgre::RigidBodyState(mNinjaNode);
	
	//Create the Body.
	btRigidBody::btRigidBodyConstructionInfo ninjaConstructionInfo(mass, ninjaState, mNinjaShape, inertia);
	//ninjaConstructionInfo.m_friction = 2;
	mNinjaBody = new btRigidBody(ninjaConstructionInfo);
	phyWorld->addRigidBody(mNinjaBody);
	
	mNinjaBody->setDamping(0.1, 0.1);
	
	mKeyboard->setEventCallback(&mInputHandler);
	mMouse->setEventCallback(&mInputHandler);
	
	Ogre::Timer frameTimer;
	//std::cout << "\n\n";
    while(!lWindow->isClosed() && continue_looping)
    {
        lWindow->update(false);
        
        bool lVerticalSynchro = true;
		lWindow->swapBuffers(/*lVerticalSynchro*/);
		
		mKeyboard->capture();
		mMouse->capture();
		
		float frameTime = float(frameTimer.getMilliseconds()) / 1000.0f;
		
		//std::cout << "Milliseconds simce last frame: " << frameTimer.getMilliseconds() << std::endl;
		mChara->addTime(frameTime);
		frameTimer.reset();
	    
	    //Update Bullet world. Don't forget the debugDrawWorld() part!
	    phyWorld->stepSimulation(frameTime, 10);
	    phyWorld->debugDrawWorld();

	    //Shows debug if F3 key down.
	    dbgdraw->setDebugMode(mKeyboard->isKeyDown(OIS::KC_F3));
	    //dbgdraw->setDebugMode(true);
	    dbgdraw->step();
		
		lRoot->renderOneFrame();
		
        Ogre::WindowEventUtilities::messagePump();
    }
	//std::cout << "\n\n";
    
    {
		// clean up character controller and the floor mesh
		if (mChara)
        {
            delete mChara;
            mChara = 0;
        }
		MeshManager::getSingleton().remove("floor");
		
		mInputManager->destroyInputObject(mKeyboard);
		mKeyboard = NULL;
		mInputManager->destroyInputObject(mMouse);
		mMouse = NULL;
		OIS::InputManager::destroyInputSystem(mInputManager);
		mInputManager = NULL;
		
		lWindow->removeAllViewports();
		
		mSceneMgr->destroyAllCameras();
		mSceneMgr->destroyAllManualObjects();
		mSceneMgr->destroyAllEntities();
		
		//Free Bullet stuff.
		phyWorld->removeCollisionObject(m_ghostObject);
	    delete m_ghostObject;
	    delete m_character;
	    
	    delete capsule;
	    
	    /*delete groundShape;
	    delete groundMotionState;
	    delete groundRigidBody;*/
	    
	    phyWorld->removeRigidBody(mNinjaBody);
	    delete ninjaState;
	    delete mNinjaBody;
	    delete mNinjaShape;
	    
	    phyWorld->removeRigidBody(mGroundBody);
	    //delete mGroundEntity;
	    delete mGroundBody->getMotionState();
	    delete mGroundBody;
	    delete mGroundShape->getMeshInterface();
	    delete mGroundShape;
	    
	    delete dbgdraw;
	    delete phyWorld;
	    
	    delete mSolver;
	    delete mDispatcher;
	    delete mCollisionConfig;
	    delete mBroadphase;
	}
    
    Ogre::LogManager::getSingleton().logMessage("end of the program");
}
