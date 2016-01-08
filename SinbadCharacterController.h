#ifndef __Sinbad_H__
#define __Sinbad_H__

#include "Ogre.h"
#include "OIS.h"

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

//#include <iostream>

using namespace Ogre;

#define NUM_ANIMS 13           // number of animations the character has
#define CHAR_HEIGHT 5          // height of character's center of mass above ground
#define CAM_HEIGHT 2           // height of camera above character's center of mass
#define RUN_SPEED 3.4f           // character running speed in units per second
#define TURN_SPEED 500.0f      // character turning in degrees per second
#define ANIM_FADE_SPEED 7.5f   // animation crossfade speed in % of full weight per second
#define JUMP_ACCEL 30.0f       // character jump acceleration in upward units per squared second
#define GRAVITY 90.0f          // gravity in downward units per squared second

class SinbadCharacterController
{
private:

    btKinematicCharacterController* mCharacterController;
	// all the animations our character has, and a null ID
	// some of these affect separate body parts and will be blended together
	enum AnimID
	{
		ANIM_IDLE_BASE,
		ANIM_IDLE_TOP,
		ANIM_RUN_BASE,
		ANIM_RUN_TOP,
		ANIM_HANDS_CLOSED,
		ANIM_HANDS_RELAXED,
		ANIM_DRAW_SWORDS,
		ANIM_SLICE_VERTICAL,
		ANIM_SLICE_HORIZONTAL,
		ANIM_DANCE,
		ANIM_JUMP_START,
		ANIM_JUMP_LOOP,
		ANIM_JUMP_END,
		ANIM_NONE
	};

public:
	
	SinbadCharacterController(Camera* cam, btKinematicCharacterController* characterController, btDynamicsWorld* dynWorld) :
	    mCharacterController(characterController), mWorld(dynWorld)//, mNewCameraDistanceChange(0)
	{
		setupBody(cam->getSceneManager());
		setupCamera(cam);
		setupAnimations();
	}

	void addTime(Real deltaTime)
	{
		updateBody(deltaTime);
		updateAnimations(deltaTime);
		updateCamera(deltaTime);
	}

	void injectKeyDown(const OIS::KeyEvent& evt)
	{
		if (evt.key == OIS::KC_Q && (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP))
		{
			// take swords out (or put them back, since it's the same animation but reversed)
			setTopAnimation(ANIM_DRAW_SWORDS, true);
			mTimer = 0;
		}
		else if (evt.key == OIS::KC_E && !mSwordsDrawn)
		{
			if (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP)
			{
				// start dancing
				setBaseAnimation(ANIM_DANCE, true);
				setTopAnimation(ANIM_NONE);
				// disable hand animation because the dance controls hands
				mAnims[ANIM_HANDS_RELAXED]->setEnabled(false);
			}
			else if (mBaseAnimID == ANIM_DANCE)
			{
				// stop dancing
				setBaseAnimation(ANIM_IDLE_BASE);
				setTopAnimation(ANIM_IDLE_TOP);
				// re-enable hand animation
				mAnims[ANIM_HANDS_RELAXED]->setEnabled(true);
			}
		}

		// keep track of the player's intended direction
		else if (evt.key == OIS::KC_W) mKeyDirection.z = -1;
		else if (evt.key == OIS::KC_A) mKeyDirection.x = -1;
		else if (evt.key == OIS::KC_S) mKeyDirection.z = 1;
		else if (evt.key == OIS::KC_D) mKeyDirection.x = 1;

		else if (evt.key == OIS::KC_SPACE && (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP)
		    && mCharacterController->canJump())
		{
			// jump if on ground
			setBaseAnimation(ANIM_JUMP_START, true);
			setTopAnimation(ANIM_NONE);
			mTimer = 0;
		}

		if (!mKeyDirection.isZeroLength() && mBaseAnimID == ANIM_IDLE_BASE)
		{
			// start running if not already moving and the player wants to move
			setBaseAnimation(ANIM_RUN_BASE, true);
			if (mTopAnimID == ANIM_IDLE_TOP) setTopAnimation(ANIM_RUN_TOP, true);
		}
	}

	void injectKeyUp(const OIS::KeyEvent& evt)
	{
		// keep track of the player's intended direction
		if (evt.key == OIS::KC_W && mKeyDirection.z == -1) mKeyDirection.z = 0;
		else if (evt.key == OIS::KC_A && mKeyDirection.x == -1) mKeyDirection.x = 0;
		else if (evt.key == OIS::KC_S && mKeyDirection.z == 1) mKeyDirection.z = 0;
		else if (evt.key == OIS::KC_D && mKeyDirection.x == 1) mKeyDirection.x = 0;

		if (mKeyDirection.isZeroLength() && mBaseAnimID == ANIM_RUN_BASE)
		{
			// stop running if already moving and the player doesn't want to move
			setBaseAnimation(ANIM_IDLE_BASE);
			if (mTopAnimID == ANIM_RUN_TOP) setTopAnimation(ANIM_IDLE_TOP);
		}
	}

#if (OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS) || (OGRE_PLATFORM == OGRE_PLATFORM_ANDROID)
	void injectMouseMove(const OIS::MultiTouchEvent& evt)
	{
		// update camera goal based on mouse movement
		updateCameraGoal(-0.05f * evt.state.X.rel, -0.05f * evt.state.Y.rel, -0.0005f * evt.state.Z.rel);
	}

	void injectMouseDown(const OIS::MultiTouchEvent& evt)
	{
		if (mSwordsDrawn && (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP))
		{
			// if swords are out, and character's not doing something weird, then SLICE!
            setTopAnimation(ANIM_SLICE_VERTICAL, true);
			mTimer = 0;
		}
	}
#else
	void injectMouseMove(const OIS::MouseEvent& evt)
	{
		// update camera goal based on mouse movement
		//updateCameraGoal(-0.05f * evt.state.X.rel, -0.05f * evt.state.Y.rel, -0.0005f * evt.state.Z.rel);
		updateCameraGoal(-0.2f * evt.state.X.rel, -0.2f * evt.state.Y.rel, -0.0005f * evt.state.Z.rel);
	}

	void injectMouseDown(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (mSwordsDrawn && (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP))
		{
			// if swords are out, and character's not doing something weird, then SLICE!
			if (id == OIS::MB_Left) setTopAnimation(ANIM_SLICE_VERTICAL, true);
			else if (id == OIS::MB_Right) setTopAnimation(ANIM_SLICE_HORIZONTAL, true);
			mTimer = 0;
		}
	}
#endif

private:

	void setupBody(SceneManager* sceneMgr)
	{
		// create main model
		//mBodyNode = sceneMgr->getRootSceneNode()->createChildSceneNode(Vector3::UNIT_Y * CHAR_HEIGHT);
		mDirectionNode = sceneMgr->getRootSceneNode()->createChildSceneNode(Vector3::UNIT_Y * CHAR_HEIGHT);
		mBodyNode = mDirectionNode->createChildSceneNode();
		mBodyEnt = sceneMgr->createEntity("SinbadBody", "Sinbad.mesh");
		mBodyNode->attachObject(mBodyEnt);
		
		mBodyNode->scale(0.2f,0.2f,0.2f);

		// create swords and attach to sheath
		LogManager::getSingleton().logMessage("Creating swords");
		mSword1 = sceneMgr->createEntity("SinbadSword1", "Sword.mesh");
		mSword2 = sceneMgr->createEntity("SinbadSword2", "Sword.mesh");
		mBodyEnt->attachObjectToBone("Sheath.L", mSword1);
		mBodyEnt->attachObjectToBone("Sheath.R", mSword2);

		/*LogManager::getSingleton().logMessage("Creating the chains");
		// create a couple of ribbon trails for the swords, just for fun
		NameValuePairList params;
		params["numberOfChains"] = "2";
		params["maxElements"] = "80";
		mSwordTrail = (RibbonTrail*)sceneMgr->createMovableObject("RibbonTrail", &params);
		mSwordTrail->setMaterialName("Examples/LightRibbonTrail");
		mSwordTrail->setTrailLength(20);
		mSwordTrail->setVisible(false);
		sceneMgr->getRootSceneNode()->attachObject(mSwordTrail);


		for (int i = 0; i < 2; i++)
		{
			mSwordTrail->setInitialColour(i, 1, 0.8, 0);
			mSwordTrail->setColourChange(i, 0.75, 1.25, 1.25, 1.25);
			mSwordTrail->setWidthChange(i, 1);
			mSwordTrail->setInitialWidth(i, 0.5);
		}*/

		mKeyDirection = Vector3::ZERO;
		mVerticalVelocity = 0;
		
		
		wasInAir = false;
		//mTranslatorNode = sceneMgr->getRootSceneNode()->createChildSceneNode(Vector3::UNIT_Y * CHAR_HEIGHT);
	}

	void setupAnimations()
	{
		// this is very important due to the nature of the exported animations
		mBodyEnt->getSkeleton()->setBlendMode(ANIMBLEND_CUMULATIVE);

		String animNames[] =
		{"IdleBase", "IdleTop", "RunBase", "RunTop", "HandsClosed", "HandsRelaxed", "DrawSwords",
		"SliceVertical", "SliceHorizontal", "Dance", "JumpStart", "JumpLoop", "JumpEnd"};

		// populate our animation list
		for (int i = 0; i < NUM_ANIMS; i++)
		{
			mAnims[i] = mBodyEnt->getAnimationState(animNames[i]);
			mAnims[i]->setLoop(true);
			mFadingIn[i] = false;
			mFadingOut[i] = false;
		}

		// start off in the idle state (top and bottom together)
		setBaseAnimation(ANIM_IDLE_BASE);
		setTopAnimation(ANIM_IDLE_TOP);

		// relax the hands since we're not holding anything
		mAnims[ANIM_HANDS_RELAXED]->setEnabled(true);

		mSwordsDrawn = false;
	}

	void setupCamera(Camera* cam)
	{
		// create a pivot at roughly the character's shoulder
		mCameraPivot = mDirectionNode->createChildSceneNode(Vector3(0,0.5f,0));
		
		mCameraNode = mCameraPivot->createChildSceneNode(Vector3(0,0,-5));
		
		//mCameraTarget = mBodyNode->createChildSceneNode(Vector3(0,2,0));
		
		desiredDistance = 
		    mCameraPivot->_getDerivedPosition().distance(mCameraNode->_getDerivedPosition());

		mCameraPivot->setFixedYawAxis(true);
		mCameraNode->setFixedYawAxis(true);

		// our model is quite small, so reduce the clipping planes
		cam->setNearClipDistance(0.1);
		cam->setFarClipDistance(100);
		mCameraNode->attachObject(cam);

		mPivotPitch = 0;
		
		mCameraNode->lookAt(mCameraPivot->_getDerivedPosition(), Node::TS_WORLD);
	}

	void updateBody(Real deltaTime)
	{
		mGoalDirection = Vector3::ZERO;   // we will calculate this

		if (mKeyDirection != Vector3::ZERO && mBaseAnimID != ANIM_DANCE)
		{
			// calculate actually goal direction in world based on player's key directions
			mGoalDirection += mKeyDirection.z * mCameraNode->_getDerivedOrientation().zAxis();
			mGoalDirection += mKeyDirection.x * mCameraNode->_getDerivedOrientation().xAxis();
			mGoalDirection.y = 0;
			mGoalDirection.normalise();
            
			
			Quaternion toGoal = mBodyNode->_getDerivedOrientation().zAxis().getRotationTo(mGoalDirection);

			// calculate how much the character has to turn to face goal direction
			Real yawToGoal = toGoal.getYaw().valueDegrees();
			// this is how much the character CAN turn this frame
			Real yawAtSpeed = yawToGoal / Math::Abs(yawToGoal) * deltaTime * TURN_SPEED;
			// reduce "turnability" if we're in midair
			if (mBaseAnimID == ANIM_JUMP_LOOP) yawAtSpeed *= 0.2f;

			// turn as much as we can, but not more than we need to
			if (yawToGoal < 0) yawToGoal = std::min<Real>(0, std::max<Real>(yawToGoal, yawAtSpeed)); //yawToGoal = Math::Clamp<Real>(yawToGoal, yawAtSpeed, 0);
			else if (yawToGoal > 0) yawToGoal = std::max<Real>(0, std::min<Real>(yawToGoal, yawAtSpeed)); //yawToGoal = Math::Clamp<Real>(yawToGoal, 0, yawAtSpeed);
			
			mBodyNode->yaw(Degree(yawToGoal));
			
			//Move it!
			mCharacterController->setWalkDirection(BtOgre::Convert::toBullet(RUN_SPEED * deltaTime * mAnims[mBaseAnimID]->getWeight() * mGoalDirection));
		}
		else
        {
            mCharacterController->setWalkDirection(btVector3(0,0,0));
        }
			
	    mDirectionNode->_setDerivedPosition(BtOgre::Convert::toOgre(
	    mCharacterController->getGhostObject()->getWorldTransform().getOrigin()));
	    
		if (mBaseAnimID == ANIM_JUMP_LOOP || !mCharacterController->onGround())
		{
			// if we're jumping, add a vertical offset too, and apply gravity
			//mBodyNode->translate(0, mVerticalVelocity * deltaTime, 0, Node::TS_LOCAL);
			//mVerticalVelocity -= GRAVITY * deltaTime;
			
			//setBaseAnimation(ANIM_JUMP_LOOP);
			
			//Vector3 pos = mBodyNode->getPosition();
			if (/*pos.y <= CHAR_HEIGHT*/ mCharacterController->onGround())
			{
				// if we've hit the ground, change to landing state
				//pos.y = CHAR_HEIGHT;
				//mBodyNode->setPosition(pos);
				setBaseAnimation(ANIM_JUMP_END, true);
				mTimer = 0;
			}
		}
		
		if (!mCharacterController->onGround() && !wasInAir)
		{
			setBaseAnimation(ANIM_JUMP_LOOP, true);
			setTopAnimation(ANIM_NONE);
			//mTimer = 0;
		}
		
		if (!mCharacterController->onGround())
		    wasInAir = true;
	    
		if (mCharacterController->onGround())
		    wasInAir = false;
	}

	void updateAnimations(Real deltaTime)
	{
		Real baseAnimSpeed = 1;
		Real topAnimSpeed = 1;

		mTimer += deltaTime;

		if (mTopAnimID == ANIM_DRAW_SWORDS)
		{
			// flip the draw swords animation if we need to put it back
			topAnimSpeed = mSwordsDrawn ? -1 : 1;

			// half-way through the animation is when the hand grasps the handles...
			if (mTimer >= mAnims[mTopAnimID]->getLength() / 2 &&
				mTimer - deltaTime < mAnims[mTopAnimID]->getLength() / 2)
			{
				// so transfer the swords from the sheaths to the hands
				mBodyEnt->detachAllObjectsFromBone();
				mBodyEnt->attachObjectToBone(mSwordsDrawn ? "Sheath.L" : "Handle.L", mSword1);
				mBodyEnt->attachObjectToBone(mSwordsDrawn ? "Sheath.R" : "Handle.R", mSword2);
				// change the hand state to grab or let go
				mAnims[ANIM_HANDS_CLOSED]->setEnabled(!mSwordsDrawn);
				mAnims[ANIM_HANDS_RELAXED]->setEnabled(mSwordsDrawn);

				// toggle sword trails
				/*if (mSwordsDrawn)
				{
					mSwordTrail->setVisible(false);
					mSwordTrail->removeNode(mSword1->getParentNode());
					mSwordTrail->removeNode(mSword2->getParentNode());
				}
				else
				{
					mSwordTrail->setVisible(true);
					mSwordTrail->addNode(mSword1->getParentNode());
					mSwordTrail->addNode(mSword2->getParentNode());
				}*/
			}

			if (mTimer >= mAnims[mTopAnimID]->getLength())
			{
				// animation is finished, so return to what we were doing before
				if (mBaseAnimID == ANIM_IDLE_BASE) setTopAnimation(ANIM_IDLE_TOP);
				else
				{
					setTopAnimation(ANIM_RUN_TOP);
					mAnims[ANIM_RUN_TOP]->setTimePosition(mAnims[ANIM_RUN_BASE]->getTimePosition());
				}
				mSwordsDrawn = !mSwordsDrawn;
			}
		}
		else if (mTopAnimID == ANIM_SLICE_VERTICAL || mTopAnimID == ANIM_SLICE_HORIZONTAL)
		{
			if (mTimer >= mAnims[mTopAnimID]->getLength())
			{
				// animation is finished, so return to what we were doing before
				if (mBaseAnimID == ANIM_IDLE_BASE) setTopAnimation(ANIM_IDLE_TOP);
				else
				{
					setTopAnimation(ANIM_RUN_TOP);
					mAnims[ANIM_RUN_TOP]->setTimePosition(mAnims[ANIM_RUN_BASE]->getTimePosition());
				}
			}

			// don't sway hips from side to side when slicing. that's just embarrassing.
			if (mBaseAnimID == ANIM_IDLE_BASE) baseAnimSpeed = 0;
		}
		else if (mBaseAnimID == ANIM_JUMP_START)
		{
			if (mTimer >= mAnims[mBaseAnimID]->getLength())
			{
				// takeoff animation finished, so time to leave the ground!
				setBaseAnimation(ANIM_JUMP_LOOP, true);
				// apply a jump acceleration to the character
				//mVerticalVelocity = JUMP_ACCEL;
			
			    mCharacterController->jump();
			}
		}
		else if (mBaseAnimID == ANIM_JUMP_END)
		{
			if (mTimer >= mAnims[mBaseAnimID]->getLength())
			{
				// safely landed, so go back to running or idling
				if (mKeyDirection == Vector3::ZERO)
				{
					setBaseAnimation(ANIM_IDLE_BASE);
					setTopAnimation(ANIM_IDLE_TOP);
				}
				else
				{
					setBaseAnimation(ANIM_RUN_BASE, true);
					setTopAnimation(ANIM_RUN_TOP, true);
				}
			}
		}

		// increment the current base and top animation times
		if (mBaseAnimID != ANIM_NONE) mAnims[mBaseAnimID]->addTime(deltaTime * baseAnimSpeed);
		if (mTopAnimID != ANIM_NONE) mAnims[mTopAnimID]->addTime(deltaTime * topAnimSpeed);

		// apply smooth transitioning between our animations
		fadeAnimations(deltaTime);
	}

	void fadeAnimations(Real deltaTime)
	{
		for (int i = 0; i < NUM_ANIMS; i++)
		{
			if (mFadingIn[i])
			{
				// slowly fade this animation in until it has full weight
				Real newWeight = mAnims[i]->getWeight() + deltaTime * ANIM_FADE_SPEED;
				mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
				if (newWeight >= 1) mFadingIn[i] = false;
			}
			else if (mFadingOut[i])
			{
				// slowly fade this animation out until it has no weight, and then disable it
				Real newWeight = mAnims[i]->getWeight() - deltaTime * ANIM_FADE_SPEED;
				mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
				if (newWeight <= 0)
				{
					mAnims[i]->setEnabled(false);
					mFadingOut[i] = false;
				}
			}
		}
	}

	void updateCamera(Real deltaTime)
	{
		// place the camera pivot roughly at the character's shoulder
		//mCameraPivot->setPosition(mBodyNode->getPosition() + Vector3::UNIT_Y * CAM_HEIGHT);
		// move the camera smoothly to the goal
		//Vector3 goalOffset = mCameraGoal->_getDerivedPosition() - mCameraNode->getPosition();
		//mCameraNode->translate(goalOffset * deltaTime * 9.0f);
		// always look at the pivot
		//mCameraNode->lookAt(mCameraPivot->_getDerivedPosition()
		//    /*mCameraTarget->_getDerivedPosition()*/, Node::TS_WORLD);
		
		
		Real currentDistance = 
		    mCameraPivot->_getDerivedPosition().distance(mCameraNode->_getDerivedPosition());
		
		//mNewCameraDistanceChange = 0;
		
		Real moveDistance = 0;
		
		/*if(desiredDistance != currentDistance)
		{
		    //mCameraNode->translate(0, 0, desiredDistance - currentDistance, Node::TS_LOCAL);
		    
		    //Real cameraTranslate = desiredDistance - currentDistance * -2 * deltaTime;
	        
	        Real cameraSpeed = 2.0f * deltaTime;
	        Math::Clamp<Real>(cameraSpeed, 0, 1);
	        
	        moveDistance = desiredDistance - currentDistance;
	        
	        //if(moveDistance > 1)
	            moveDistance *= cameraSpeed;
	        //else
	        //    moveDistance = 0;
	        //std::cerr << "dd" << moveDistance << '\n';
	        
	        //mCameraNode->translate(0, 0, moveDistance, Node::TS_LOCAL);
	        
	        //mCameraNode->translate(0, 0, cameraTranslate, Node::TS_LOCAL);
		}*/
		
		{
            //use the convex sweep test to find a safe position for the camera (not blocked by static geometry)
            //btVector3 m_cameraPosition = BtOgre::Convert::toBullet(mCameraNode->_getDerivedPosition());
            
	        btSphereShape cameraSphere(0.2f);
	        btTransform cameraFrom,cameraTo;
	        cameraFrom.setIdentity();
	        cameraFrom.setOrigin(/*characterWorldTrans.getOrigin()*/
	            BtOgre::Convert::toBullet(/*mCameraTarget*/mCameraPivot->_getDerivedPosition()));
	        cameraTo.setIdentity();
	        //cameraTo.setOrigin(m_cameraPosition);
	        cameraTo.setOrigin(BtOgre::Convert::toBullet(
	            mCameraNode->convertLocalToWorldPosition(
	                Vector3(0,0,desiredDistance - currentDistance))));
	        
	        btVector3 m_cameraPosition = cameraTo.getOrigin();
	        
	        btCollisionWorld::ClosestConvexResultCallback cb( /*characterWorldTrans*/cameraFrom.getOrigin(), cameraTo.getOrigin() );
	        cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
		    
	        mWorld->convexSweepTest(&cameraSphere,cameraFrom,cameraTo,cb);
	        if (cb.hasHit())
	        {
		        btScalar minFraction  = cb.m_closestHitFraction;//btMax(btScalar(0.3),cb.m_closestHitFraction);
		        m_cameraPosition.setInterpolate3(cameraFrom.getOrigin(),cameraTo.getOrigin(),minFraction);
		        
		        //mCameraNode->_setDerivedPosition(BtOgre::Convert::toOgre(m_cameraPosition));
		        Vector3 cameraMovement = mCameraPivot->convertWorldToLocalPosition(
		            BtOgre::Convert::toOgre(m_cameraPosition)) - mCameraNode->getPosition();
		        
		        moveDistance = cameraMovement.z;
		        
		        //mCameraNode->setPosition(newCameraPositionLocal);
		        
		        Real cameraSpeed = 15.0f * deltaTime;
		        Math::Clamp<Real>(cameraSpeed, 0, 1);
		        
		        //moveDistance =
		        //    mCameraNode->_getDerivedPosition().distance(
		        //        BtOgre::Convert::toOgre(m_cameraPosition));
		        
		        //moveDistance = mCameraPivot->_getDerivedPosition().distance(
		        //    mCameraPivot->convertWorldToLocalPosition(
		        //        BtOgre::Convert::toOgre(m_cameraPosition))) - currentDistance;
		        
		        //if(moveDistance > 0.1f)
		            moveDistance *= cameraSpeed;
		        
		        //moveDistance *= -1;
		        //mCameraNode->translate(0, 0, moveDistance * -1, Node::TS_LOCAL);
		        
		        mCameraNode->translate(0,0,moveDistance);
	        }
	        else
	        {
	            //mCameraNode->translate(0, 0, desiredDistance - currentDistance, Node::TS_LOCAL);
	            
	            moveDistance = desiredDistance - currentDistance;
		        
		        Real cameraSpeed = 15.0f * deltaTime;
		        Math::Clamp<Real>(cameraSpeed, 0, 1);
		        
		        moveDistance *= cameraSpeed;
		        
		        mCameraNode->translate(0, 0, moveDistance, Node::TS_LOCAL);
	        }
	    }
	    
	    //mCameraNode->translate(0, 0, moveDistance, Node::TS_LOCAL);
	}

	void updateCameraGoal(Real deltaYaw, Real deltaPitch, Real deltaZoom)
	{
	    deltaPitch *= -1;
		if(mKeyDirection != Vector3::ZERO && mBaseAnimID != ANIM_DANCE)
		    mDirectionNode->yaw(Degree(deltaYaw), Node::TS_WORLD);
		else
		    mCameraPivot->yaw(Degree(deltaYaw), Node::TS_WORLD);
        
		
		//mPivotPitch = mCameraPivot->getOrientation().getPitch().valueDegrees();
		
		// bound the pitch
		if (!(mPivotPitch + deltaPitch > /*25*/ 60 && deltaPitch > 0) &&
			!(mPivotPitch + deltaPitch < -/*60*/60 && deltaPitch < 0))
		{
			mCameraPivot->pitch(Degree(deltaPitch), Node::TS_LOCAL);
			mPivotPitch += deltaPitch;
		}
		
		//Real dist = mCameraGoal->_getDerivedPosition().distance(mCameraPivot->_getDerivedPosition());
		Real dist = mCameraNode->_getDerivedPosition().distance(mCameraPivot->_getDerivedPosition()
		    /*mCameraTarget->_getDerivedPosition()*/);
		Real distChange = deltaZoom * dist;

		// bound the zoom
		if (!(dist + distChange < 2 && distChange < 0) &&
			!(dist + distChange > 10 && distChange > 0))
		{
			//mCameraGoal->translate(0, 0, distChange, Node::TS_LOCAL);
			mCameraNode->translate(0, 0, distChange, Node::TS_LOCAL);
			
			if(deltaZoom != 0)
			{
			    desiredDistance = 
		            mCameraPivot->_getDerivedPosition().distance(mCameraNode->_getDerivedPosition());
		        
		        //desiredDistance += distChange;
		    }
		}
	}

	void setBaseAnimation(AnimID id, bool reset = false)
	{
		if (mBaseAnimID >= 0 && mBaseAnimID < NUM_ANIMS)
		{
			// if we have an old animation, fade it out
			mFadingIn[mBaseAnimID] = false;
			mFadingOut[mBaseAnimID] = true;
		}

		mBaseAnimID = id;

		if (id != ANIM_NONE)
		{
			// if we have a new animation, enable it and fade it in
			mAnims[id]->setEnabled(true);
			mAnims[id]->setWeight(0);
			mFadingOut[id] = false;
			mFadingIn[id] = true;
			if (reset) mAnims[id]->setTimePosition(0);
		}
	}

	void setTopAnimation(AnimID id, bool reset = false)
	{
		if (mTopAnimID >= 0 && mTopAnimID < NUM_ANIMS)
		{
			// if we have an old animation, fade it out
			mFadingIn[mTopAnimID] = false;
			mFadingOut[mTopAnimID] = true;
		}

		mTopAnimID = id;

		if (id != ANIM_NONE)
		{
			// if we have a new animation, enable it and fade it in
			mAnims[id]->setEnabled(true);
			mAnims[id]->setWeight(0);
			mFadingOut[id] = false;
			mFadingIn[id] = true;
			if (reset) mAnims[id]->setTimePosition(0);
		}
	}

	Camera* mCamera;
	SceneNode* mBodyNode;
	SceneNode* mCameraPivot;
	//SceneNode* mCameraGoal;
	SceneNode* mCameraNode;
	Real mPivotPitch;
	Entity* mBodyEnt;
	Entity* mSword1;
	Entity* mSword2;
	//RibbonTrail* mSwordTrail;
	AnimationState* mAnims[NUM_ANIMS];    // master animation list
	AnimID mBaseAnimID;                   // current base (full- or lower-body) animation
	AnimID mTopAnimID;                    // current top (upper-body) animation
	bool mFadingIn[NUM_ANIMS];            // which animations are fading in
	bool mFadingOut[NUM_ANIMS];           // which animations are fading out
	bool mSwordsDrawn;
	Vector3 mKeyDirection;      // player's local intended direction based on WASD keys
	Vector3 mGoalDirection;     // actual intended direction in world-space
	Real mVerticalVelocity;     // for jumping
	Real mTimer;                // general timer to see how long animations have been playing
	
	SceneNode* mDirectionNode;
	bool wasInAir;
	btDynamicsWorld* mWorld;
	Real desiredDistance;
	//SceneNode* mCameraTarget;
	//Real mNewCameraDistanceChange;    
};

#endif
