#include <cmath>
#include <iostream>
#include "NcStateMachine\G94.h"
#include "NcGeomKernel\NcProfile.h"
#include "NcStateMachine\NcToolController.h"
#include <QtOpenGL>
#include <QList>
using namespace std;
using namespace DiscreteSimulator;

G94::G94()   //canned facing cycle
{
	noOfDL = 0;
	mPartProfileIndex = 0;
	mPartProfileList = new QList<Profile *>;

	connect(this, SIGNAL(updateToolPathDisplay(int)), 
		NcToolController::getToolControllerInstance(), SLOT(updateTPDisplay(int)));

	connect(this, SIGNAL(updateCycleTime(int, int, int)),
		    NcToolController::getToolControllerInstance(), SLOT(updateCycleTime(int, int, int)));
}

void	G94::reinitializeCode()
{
	noOfDL = 0;
	mPartProfileIndex = 0;
}


//G94::~G94()
//{
//}



STATUS	G94::generateDisplayList()
{
	Profile *gProfile = new Profile();
	gProfile->type = CG94;
	gProfile->typeTool = CT05;
	mPartProfileList->push_back(gProfile);

	//time in seconds taking units as turning length = mm, feedrate = mm/rev
	//spindle speed = rev / min
	double facinglength = fabs(fabs(X) - fabs(mStartX));
	double feed = F ;
	double sspeed = mSpindleSpeed;
	//gives turning time in minutes ---> has been converted to seconds 
	mCodeExecutionTime =  ( facinglength / (F * mSpindleSpeed) ) * 60;

	mTotalCycleTime = mTotalCycleTime + mCodeExecutionTime;

	mHours = (int)(mTotalCycleTime / 3600);
	int min = mTotalCycleTime % 3600;
	mMinutes = (int)(min / 60);
	int sec = min % 60;
	mSeconds = sec;

	int no_pts = (1 + MAX_LINEAR_SUBDIV);
	//profile->allocate();
	vector<NcVector> profile(no_pts);
	profile[0]=NcVector(mStartZ,mStartX,0);
	profile[MAX_LINEAR_SUBDIV] = NcVector(Z,X,0);
	/*profile->P[0][1] = mStartX;
	profile->P[0][2] = 0;

	profile->P[MAX_LINEAR_SUBDIV][0] = Z;
	profile->P[MAX_LINEAR_SUBDIV][1] = X;
	profile->P[MAX_LINEAR_SUBDIV][2] = 0;*/

	double u = 0.0;
	double du = 1.0 / (double)(MAX_LINEAR_SUBDIV - 1);

	for(int i = 1; i < MAX_LINEAR_SUBDIV; i++, u += du)
	{
		profile[i] = profile[0] * (1-u) + profile[MAX_LINEAR_SUBDIV] * (u);
	}

	GLuint newlistindex = glGenLists(1);
	//gProfile->mAssociated2DDLIndexes->push_back(newlistindex);
	gProfile->addProfileDisplayListIndex(newlistindex);
	mCumulativeDLList.push_back(newlistindex);
	mListIndex++;
	mLocalIndex = mListIndex;

	glNewList(newlistindex, GL_COMPILE);
	for(int i = 0; i < no_pts - 1; i++)
	{
	
		glColor3d(0.0, 0.0, 1.0);		//turing color
		glBegin(GL_LINES);
			glVertex3f(profile[i][0], profile[i][1], 0.0); 
			glVertex3f(profile[i+1][0], profile[i+1][1], 0.0); 		
		glEnd();
		
	}
	glEndList();

	gProfile->setProfile(profile);

	//handle opengl error here 
	return OK;
}

bool	G94::executeCode(SimulationState simstate, NcCode *code) //return true when code execution is complete else return false
{
	if(simstate == RUNNING)
	{
		/*cout << "G94 profile index: " << mPartProfileIndex << endl;*/
		if(mPartProfileIndex < mPartProfileList->size())
		{
			if(noOfDL < mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLsize())
			{
				/*cout << "G94 DL no in noOfDL before executing: " << noOfDL << endl;*/
				noOfDL++;
				NcToolController::getToolControllerInstance()
					->updateToolPosition(mPartProfileList->at(mPartProfileIndex)->P[noOfDL][0],
										 mPartProfileList->at(mPartProfileIndex)->P[noOfDL][1],
										 mPartProfileList->at(mPartProfileIndex)->P[noOfDL][2]);
				noOfDL--;

				glCallList(mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL));

				mLastExecutedDL = mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL);

				for(int i = 0; i < mLocalIndex; i++)
				{
					glCallList(mCumulativeDLList.at(i));
				}

				emit updateToolPathDisplay(mLocalIndex);
				noOfDL++;

				/*cout << "G94 DL no in noOfDL after executing: " << noOfDL << endl;*/
				return false;
			}
			else
			{
				--noOfDL;
				/*cout << "G94 DL no in noOfDL else before: " << noOfDL << endl;*/

				NcToolController::getToolControllerInstance()
					->updateToolPosition(mPartProfileList->at(mPartProfileIndex)->P[mPartProfileList->at(mPartProfileIndex)->no_pts - 1][0],
										 mPartProfileList->at(mPartProfileIndex)->P[mPartProfileList->at(mPartProfileIndex)->no_pts - 1][1],
										 mPartProfileList->at(mPartProfileIndex)->P[mPartProfileList->at(mPartProfileIndex)->no_pts - 1][2]);

				glCallList(mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL));

				mLastExecutedDL = mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL);

				for(int i = 0; i < mLocalIndex; i++)
				{
					glCallList(mCumulativeDLList.at(i));
				}
				
				mPartProfileIndex++;
				noOfDL = 0;
				/*cout << "G94 DL no in noOfDL else after: " << noOfDL << endl;*/
				return false;
			}
		}
		else
		{
			/*cout << "G94 End of profile list: " << endl;*/

			NcToolController::getToolControllerInstance()
					->updateToolPosition(mPartProfileList->at(mPartProfileList->size() - 1)->P[mPartProfileList->at(mPartProfileList->size() - 1)->no_pts - 1][0],
										 mPartProfileList->at(mPartProfileList->size() - 1)->P[mPartProfileList->at(mPartProfileList->size() - 1)->no_pts - 1][1],
										 mPartProfileList->at(mPartProfileList->size() - 1)->P[mPartProfileList->at(mPartProfileList->size() - 1)->no_pts - 1][2]);

			glCallList(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLsize() - 1));

			mLastExecutedDL = mPartProfileList->at(mPartProfileList->size() - 1)
								->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
								->getAssocitedDBDLsize() - 1);

			for(int i = 0; i < mLocalIndex; i++)
			{
				glCallList(mCumulativeDLList.at(i));
			}
			
			emit updateCycleTime(mHours, mMinutes, mSeconds);

			return true;
		}
	}
	else if(simstate == END)
	{
		NcToolController::getToolControllerInstance()
					->updateToolPosition(mCycleStartZ, mCycleStartX, 0);

		glCallList(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLsize() - 1));

		mLastExecutedDL = mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLsize() - 1);

		for(int i = 0; i < mLocalIndex; i++)
		{
			glCallList(mCumulativeDLList.at(i));
		}
			
		return true;
	}
	else if(simstate == PAUSED)
	{
		NcToolController::getToolControllerInstance()
					->updateToolPosition(mPartProfileList->at(mPartProfileIndex)->P[noOfDL][0],
										 mPartProfileList->at(mPartProfileIndex)->P[noOfDL][1],
										 mPartProfileList->at(mPartProfileIndex)->P[noOfDL][2]);

		glCallList(mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL));

		mLastExecutedDL = mPartProfileList->at(mPartProfileIndex)->getAssocitedDBDLIndexes(noOfDL);

		for(int i = 0; i < mLocalIndex; i++)
		{
			glCallList(mCumulativeDLList.at(i));
		}

		return false;
	}

	return false;
}

bool	G94::executeLastDLForCode()
{
	glCallList(mPartProfileList->at(mPartProfileList->size() - 1)
				->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
				->getAssocitedDBDLsize() -1));

	for(int i = 0; i < mLocalIndex; i++)  
	{
		glCallList(mCumulativeDLList.at(i));
	}

	//used in simulation controller when simulation is paused so that last executed DL is displayed continuously
	mLastExecutedDL = mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLIndexes(mPartProfileList->at(mPartProfileList->size() - 1)
						->getAssocitedDBDLsize() -1);

	return true;
}





		
		