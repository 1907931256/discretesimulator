#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QProgressBar>
#include <QFileInfo>
#include <QTimer>
#include <QSlider>
#include <QLabel>
#include <QSettings>

#include "ui_discreteSim.h"
#include "NcUIComponents\NcSimulationWindow.h"
#include "NcUIComponents\NcToolPathWindow.h"
#include "NcFileIO\NcFileReadWriteManager.h"
#include "NcFileIO\NcFanuc0TFileReader.h"
#include "NcFileIO\NcFanuc10T11TFileReader.h"
#include "NcFileIO\NcSinumerikFileReader.h"
#include "NcFileIO\cleanupReadWriteInstance.h"
#include "NcSimulationController\NcSimulationController.h"
#include "NcSimulationController\cleanupSimulationController.h"
#include "NcUIComponents\NcStatusWindow.h"
#include "NcUIComponents\cleanupStaticWindowInstances.h"
#include "NcUIComponents\NcStockDimensionDialog.h"
#include "NcUIComponents\NcCodeEditor.h"
#include "NcUIComponents\NcMdiModeDialog.h"
#include "NcUIComponents\NcJogModeDialog.h"
#include "NcUIComponents\NcBackGroundDialog.h"
#include "NcUIComponents\NcSimulationWindow.h"
#include "NcUIComponents\NcStatusWindow.h"
#include "NcUIComponents\NcToolOffsetsDialog.h"
#include "NcUIComponents\NcMachineSetupOptions.h"
//#include "NcUIComponents\NcVideoCapture.h"
#include "NcStateMachine\cleanupNcMachineInstance.h"
#include "NcStateMachine\NcToolController.h"
#include "NcDisplay\NcDisplay.h"

#include "NcUi\NCMainWindow.h"


using namespace DiscreteSimulator;

static bool bybctrlbool;


NCMainWindow::NCMainWindow(QWidget *parent) : 
QMainWindow(parent),
m_simulationWindow(0),
m_toolPathWindow(0)
{
	mMainWindowUI = new Ui::MainWindow();
	mMainWindowUI->setupUi(this);

	/*for setting the Visibilities of windows*/
	mMainWindowUI->toolWindow->setVisible(false);
	mMainWindowUI->propertyWindow->setVisible(false);
	mMainWindowUI->CodeWindow->setVisible(false);
	mMainWindowUI->ToolPathWindow->setVisible(false);
	//mMainWindowUI->videoWindow->setVisible(false);

	createConnections();
	init();

}

void NCMainWindow::init()
{
	
	//settings is read and restored
	readSettings();

	addDockWidget(Qt::LeftDockWidgetArea,mMainWindowUI->CodeWindow);
	addDockWidget(Qt::LeftDockWidgetArea,mMainWindowUI->ToolPathWindow);
	addDockWidget(Qt::LeftDockWidgetArea,mMainWindowUI->toolWindow);
	//addDockWidget(Qt::LeftDockWidgetArea,mMainWindowUI->videoWindow);
	addDockWidget(Qt::LeftDockWidgetArea,mMainWindowUI->propertyWindow);
	

	
    mMainWindowUI->statusbar->showMessage(tr("Ready"));
 	mMainWindowUI->propertyWindow->setWidget(NcStatusWindow::StatusWindowInstance());
	mMainWindowUI->menuWindow->addAction(mMainWindowUI->propertyWindow->toggleViewAction());

	setupSimulationSpeedController();

	

	mUpdateTimer = new QTimer(this);
	createRecentFileActions();
	mSimulationRunning = false;
	mIsNewFile = true;

	
}

NCMainWindow::~NCMainWindow()
{
	/*delete mMainWindowUI;*/
}


void	NCMainWindow::createConnections()
{
	connect(mMainWindowUI->action_Open, SIGNAL(triggered()), this, SLOT(open()));
	connect(mMainWindowUI->action_Save, SIGNAL(triggered()), this, SLOT(save()));
	connect(mMainWindowUI->action_Save_as, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(mMainWindowUI->action_New, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(mMainWindowUI->actionClose, SIGNAL(triggered()), this, SLOT(closeFile()));
	connect(mMainWindowUI->action_Quit, SIGNAL(triggered()), this, SLOT(close()));
	connect(mMainWindowUI->actionStart_MDI_Mode, SIGNAL(triggered()), this, SLOT(executeMDIMode()));
	connect(mMainWindowUI->actionWorkpiece_Setup, SIGNAL(triggered()), this, SLOT(setStockDimensions()));
	connect(mMainWindowUI->actionStart_Jog_Mode, SIGNAL(triggered()), this, SLOT(executeJOGMode()));
	connect(mMainWindowUI->actionBlock_By_Block, SIGNAL(triggered()), this, SLOT(setSimulationModeToByBlock()));
	connect(mMainWindowUI->actionCycle_2, SIGNAL(triggered()), this, SLOT(setSimulationModeToByCycle()));
	connect(mMainWindowUI->actionSTL, SIGNAL(triggered()), this, SLOT(export_STL()));
	


	connect(NcSimulationController::getSimControllerInstance(), SIGNAL(codeEditorRequiresUpdate(int)),
			this, SLOT(updateCodeEditor(int)));

	connect(NcSimulationController::getSimControllerInstance(), SIGNAL(enableRewindButton()),
			this, SLOT(enableRewind()));

	connect(NcToolController::getToolControllerInstance(), SIGNAL(updateToolPosInStatusWindow(double, double, double)),
		    this, SLOT(updateToolPos(double, double, double)));

	connect(NcToolController::getToolControllerInstance(), SIGNAL(updateFeedRateInStatusWindow(int)),
			this, SLOT(updateFeedRate(int)));

	connect(NcToolController::getToolControllerInstance(), SIGNAL(updateSpeedInStatusWindow(double)),
			this, SLOT(updateSpeed(double)));

	connect(NcToolController::getToolControllerInstance(), SIGNAL(updateSpindleStat(bool)),
			this, SLOT(updateSpindleStatus(bool)));

	 connect(mMainWindowUI->actionBackground, SIGNAL(triggered()), this, SLOT(OpenBackgroundColorDialog()));

	 connect(mMainWindowUI->actionMachine_Setup, SIGNAL(triggered()), this, SLOT(OpenMacSetupOptionsDialog()));

	 connect(mMainWindowUI->actionTool_Setup, SIGNAL(triggered()), this, SLOT(OpenToolOffsetsDialog()));

	 connect(NcToolController::getToolControllerInstance(), SIGNAL(updateToolPathDisplayInTPWin(int)),
			this,	SLOT(updateToolPDis(int)));

	 connect(NcToolController::getToolControllerInstance(), SIGNAL(updateCycleTimeStatus(int, int, int)),
			 this, SLOT(updateCycleTimeInStatusWin(int, int, int)));

}


void	NCMainWindow::OpenToolOffsetsDialog()
{
	NcToolOffsetsDialog::getInstance()->exec();
}


void	NCMainWindow::OpenMacSetupOptionsDialog()
{
	NcMachineSetupOptions::getInstance()->exec();
}

void	NCMainWindow::updateCycleTimeInStatusWin(int hr, int min, int sec)
{
	NcStatusWindow::StatusWindowInstance()->updateCycleTime(hr, min, sec);
}


void	NCMainWindow::enableRewind()
{
	mMainWindowUI->actionAction_Rewind->setEnabled(true);

	connect(mMainWindowUI->actionAction_Rewind, SIGNAL(triggered()), this, SLOT(rewindSimulation()));
}


void	NCMainWindow::rewindSimulation()
{
	if(m_simulationWindow)
	{
		m_simulationWindow->makeCurrent();
		m_simulationWindow->reinitializeSimControllers();
		NcSimulationController::getSimControllerInstance()->rewindSimulation();
		mSimulationRunning = false;
		mUpdateTimer->stop();
		m_simulationWindow->updateGL();
	}

	if(m_toolPathWindow)
	{
		m_toolPathWindow->makeCurrent();
		m_toolPathWindow->reinitializeSimVariables();
		m_toolPathWindow->updateGL();
	}

	mMainWindowUI->actionRun->setEnabled(true);
	mMainWindowUI->actionAction_Rewind->setEnabled(false);
}



void	NCMainWindow::setSimulationModeToByBlock()
{
	NcSimulationController::getSimControllerInstance()->setSimulationMode(BYBLOCK);
}

void	NCMainWindow::setSimulationModeToByCycle()
{
	NcSimulationController::getSimControllerInstance()->setSimulationMode(BYCYCLE);
}


void	NCMainWindow::updateToolPDis(int index)
{
	NcDisplay::getNcDisplayInstance()->setIndex(index);
	if(m_toolPathWindow)
	{
		m_toolPathWindow->makeCurrent();
		m_toolPathWindow->updateToolPathWindow();
	}
}

void	NCMainWindow::OpenBackgroundColorDialog()
{
	NcBackGroundDialog::backgroundDialogInstance()->exec();
}


void	NCMainWindow::updateSpindleStatus(bool status)
{
	NcStatusWindow::StatusWindowInstance()->updateSpindleStatus(status);
}


void	NCMainWindow::updateSpeed(double speed)
{
	NcStatusWindow::StatusWindowInstance()->updateSpindleSpeed(speed);
}

void	NCMainWindow::updateFeedRate(int feed)
{
	NcStatusWindow::StatusWindowInstance()->updateFeedRate(feed);
}


void	NCMainWindow::updateToolPos(double mCurrentToolZPos, double mCurrentToolXPos, double mCurrentToolYPos)
{
	NcStatusWindow::StatusWindowInstance()->updateToolPosition(mCurrentToolZPos, mCurrentToolXPos, mCurrentToolYPos);
}


void	NCMainWindow::updateCodeEditor(int lineno)
{
	NcCodeEditor::NcCodeEditorInstance()->highlightCodeLineInEditor(lineno);
}


void	NCMainWindow::setStockDimensions()
{
	NcStockDimensionDialog::StockDimDialogInstance()->exec();
}

void	NCMainWindow::executeMDIMode()
{
	NcMdiModeDialog::mdiModeDialogInstance()->exec();
}

void	NCMainWindow::executeJOGMode()
{
	NcJogModeDialog::jogModeDialogInstance()->exec();
}


void	NCMainWindow::createRecentFileActions()
{
	 for (int i = 0; i < MaxRecentFiles; i++)
	 {
         recentFileActions[i] = new QAction(this);
		 recentFileActions[i]->setVisible(false);
         connect(recentFileActions[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		 mMainWindowUI->menu_File->addAction(recentFileActions[i]);
     }

	 updateRecentFileActions();
}

void	NCMainWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
	for (int i = 0; i < numRecentFiles; i++)
	{
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActions[i]->setText(text);
		recentFileActions[i]->setData(files[i]);
		recentFileActions[i]->setVisible(true);
	}
	for(int j = numRecentFiles; j < MaxRecentFiles; j++)
	{
		recentFileActions[j]->setVisible(false);
	}
}


void	NCMainWindow::openRecentFile()
{
     	QAction *action = qobject_cast<QAction *>(sender());
     	if (action)
		{
			QString fileName = action->data().toString();
			openFile(fileName);			 
		}
}

void	NCMainWindow::updateWidgets()
{
	if(m_simulationWindow)
	{
		m_simulationWindow->makeCurrent();
		m_simulationWindow->updateSimulationWindow();
	}
	if(m_toolPathWindow)
	{
		m_toolPathWindow->makeCurrent();
		m_toolPathWindow->updateGL();
	}
}



void	NCMainWindow::setCurrentFile(const QString &fileName)
{
	
    QString appName = qApp->applicationName();

	curFileName = fileName;

	NcCodeEditor::NcCodeEditorInstance()->document()->setModified(false);
	NcCodeEditor::NcCodeEditorInstance()->setWindowModified(false);
   
    QString shownName;

    if (curFileName.isEmpty())
        shownName = tr("untitled.nc*");
    else
        shownName = strippedName(curFileName);

    //appName = "DiscreteSimulatorV1.0";
    
    if(NcCodeEditor::NcCodeEditorInstance())
        setWindowTitle(tr("%1 - %2").arg(appName).arg(shownName));
    else
        setWindowTitle(tr("%1").arg(appName));

    	QSettings settings;
    	QStringList files = settings.value("recentFileList").toStringList();
    	files.removeAll(fileName);
    	files.prepend(fileName);
    	while (files.size() > MaxRecentFiles)
        	files.removeLast();

    	settings.setValue("recentFileList", files);

    	foreach (QWidget *widget, QApplication::topLevelWidgets())
		{
        	NCMainWindow *mainWin = qobject_cast<NCMainWindow *>(widget);
        	if (mainWin)
            	mainWin->updateRecentFileActions();
    	}

}

QString	NCMainWindow::strippedName(const QString &fullFileName)
{
     return QFileInfo(fullFileName).fileName();
} 


void	NCMainWindow::newFile()
{
	
	setupGLWidgetWindows();
	setupNcCodeEditor();
	setupVideoCaptureWindow();

	mMainWindowUI->CodeWindow->setVisible(true);
	mMainWindowUI->ToolPathWindow->setVisible(true);
	mMainWindowUI->propertyWindow->setVisible(true);
	//mMainWindowUI->videoWindow->setVisible(true);

	mMainWindowUI->actionBackground->setEnabled(true);
	
}

void	NCMainWindow::open()
{
    openFile();
}


bool	NCMainWindow::isSaved()
{
    if (NcCodeEditor::NcCodeEditorInstance()->document()->isModified())
     {
         QMessageBox::StandardButton ret;
         ret = QMessageBox::warning(this, tr("Application"),
                      tr("The document has been modified.\n"
                         "Do you want to save your changes?"),
                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
         if (ret == QMessageBox::Save)
             return save();
         else if (ret == QMessageBox::Cancel)
             return false;
     }
     return true;
}

bool	NCMainWindow::save()
{
     if (curFileName.isEmpty())
     {
         return saveAs();
     } 
	 else
	 {
         return saveFile(curFileName);
     }
}
   
bool	NCMainWindow::saveAs()
{
     QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), tr("./defaultDS_"),
                            tr("Fanuc 10T/11T(*.f10t);;Fanuc 0T(*.f0t)"));

     	if (fileName.isEmpty())
        	 return false;

	return saveFile(fileName);
}


bool	NCMainWindow::saveFile(const QString &fileName)
{
     QFile file(fileName);
     if (!file.open(QFile::WriteOnly | QFile::Text))
     {
         QMessageBox::warning(this, tr("Application"),
                              tr("Cannot write file %1:\n%2.")
                              .arg(fileName)
                              .arg(file.errorString()));
         return false;
     }

     QTextStream out(&file);
     QApplication::setOverrideCursor(Qt::WaitCursor);
     out << NcCodeEditor::NcCodeEditorInstance()->toPlainText();
     QApplication::restoreOverrideCursor();

     setCurrentFile(fileName);
     return true;
}



void	NCMainWindow::openFile(QString fname)
{
    /*call newFile() and then make open button grayed*/
    /*and also new button grayed*/
	mIsNewFile = false;

	if (isSaved())
	{
		QString filename;

            if(fname.isEmpty())
			{
                filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                        tr("./demotest/"),
                        tr("Fanuc 10T/11T(*.f10t);;Fanuc 0T files(*.f0t);;Sinumerik files(*.sinu)"));
            }
            else
                filename = fname;

            if (filename.isEmpty())	return;
			else
			{ 
                /*mpProgressBar->setValue(0);
                mpProgressBar->show();*/
               
				QFileInfo fileinfo(filename);
				QString ext = fileinfo.suffix();
                      
                if(ext.compare(tr("f10t")) == 0)
                {
					QApplication::setOverrideCursor(Qt::WaitCursor);
					NcFanuc10T11TFileReader::getReaderInstance()->setCurrentNcFile(filename);

					newFile();
					NcFanuc10T11TFileReader::getReaderInstance()->checkCodeSyntax();
					setCurrentFile(filename);

					NcCodeEditor::NcCodeEditorInstance()->
						setPlainText(NcFanuc10T11TFileReader::getReaderInstance()->getFullNcCodeText());								
					
					mMainWindowUI->actionRun->setEnabled(true);
					mMainWindowUI->actionNext->setEnabled(true);
					connect(mMainWindowUI->actionRun, SIGNAL(triggered()), this, SLOT(run()));
			        
					connect(mSpeedControllerSlider, SIGNAL(valueChanged(int)),
							this, SLOT(changeSimulationSpeed(int)));

					connect(mMainWindowUI->actionNext, SIGNAL(triggered()), this, SLOT(next()));
				                    
					QApplication::restoreOverrideCursor();
					
                }
				else if(ext.compare(tr("f0t")) == 0)
				{
					QApplication::setOverrideCursor(Qt::WaitCursor);
					NcFanuc0TFileReader::getReaderInstance()->setCurrentNcFile(filename);
					newFile();
					NcFanuc0TFileReader::getReaderInstance()->checkCodeSyntax();
					
					setCurrentFile(filename);

					NcCodeEditor::NcCodeEditorInstance()->
						setPlainText(NcFanuc0TFileReader::getReaderInstance()->getFullNcCodeText());

					mMainWindowUI->actionRun->setEnabled(true);
					mMainWindowUI->actionNext->setEnabled(true);
					connect(mMainWindowUI->actionRun, SIGNAL(triggered()), this, SLOT(run()));
			        
					connect(mSpeedControllerSlider, SIGNAL(valueChanged(int)),
							this, SLOT(changeSimulationSpeed(int)));

					connect(mMainWindowUI->actionNext, SIGNAL(triggered()), this, SLOT(next()));
				                    
					QApplication::restoreOverrideCursor();
					

				}
				else if(ext.compare(tr("sinu")) == 0)
				{
					QApplication::setOverrideCursor(Qt::WaitCursor);
					NcSinumerikFileReader::getReaderInstance()->setCurrentNcFile(filename);
					newFile();
					NcSinumerikFileReader::getReaderInstance()->checkCodeSyntax();
					
					setCurrentFile(filename);

					NcCodeEditor::NcCodeEditorInstance()->
						setPlainText(NcSinumerikFileReader::getReaderInstance()->getFullNcCodeText());

					mMainWindowUI->actionRun->setEnabled(true);
					mMainWindowUI->actionNext->setEnabled(true);
					connect(mMainWindowUI->actionRun, SIGNAL(triggered()), this, SLOT(run()));
			        
					connect(mSpeedControllerSlider, SIGNAL(valueChanged(int)),
							this, SLOT(changeSimulationSpeed(int)));

					connect(mMainWindowUI->actionNext, SIGNAL(triggered()), this, SLOT(next()));
				                    
					QApplication::restoreOverrideCursor();
					

				}
                else
				{
                    QMessageBox::about(this, tr("About Application"), tr("File Reading Failed"));
                }
           }
	}

	m_simulationWindow->setIsoView();
}



bool	NCMainWindow::export_STL()
{	
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), tr("./STL"), tr("*.stl *.STL"));
	
	if(fileName.isEmpty())
		return false;
	
	std::string fn = fileName.toStdString();
	const char *file = fn.c_str();

	NcFileReadWriteManager::getFileReadWriteInstance()->writeSTLFile(file);
	
	return true;
}



void    NCMainWindow::setupGLWidgetWindows()
{
	if(!m_simulationWindow)
		m_simulationWindow = new NcSimulationWindow(this);

	m_simulationWindow->setIsNewFile(mIsNewFile);
	setCentralWidget(m_simulationWindow);
    m_simulationWindow->setFocus();

	if(!m_toolPathWindow)
		m_toolPathWindow = new NcToolPathWindow(this, m_simulationWindow);
	
	mMainWindowUI->ToolPathWindow->setWidget(m_toolPathWindow);
	mMainWindowUI->menuWindow->addAction(mMainWindowUI->ToolPathWindow->toggleViewAction());


	connect(mMainWindowUI->actionFront_View, SIGNAL(triggered()), m_simulationWindow, SLOT(setFrontView()));
    connect(mMainWindowUI->actionTop_View, SIGNAL(triggered()), m_simulationWindow, SLOT(setTopView()));
    connect(mMainWindowUI->actionSide_View, SIGNAL(triggered()), m_simulationWindow, SLOT(setSideView()));
    connect(mMainWindowUI->actionIso_View, SIGNAL(triggered()), m_simulationWindow, SLOT(setIsoView()));
	connect(mMainWindowUI->actionZoomIn, SIGNAL(triggered()), m_simulationWindow, SLOT(zoomin()));
    connect(mMainWindowUI->actionZoomOut, SIGNAL(triggered()), m_simulationWindow, SLOT(zoomout()));
}




void	NCMainWindow::setupNcCodeEditor()
{
    QFont font;
    font.setFamily(tr("Courier"));
    font.setFixedPitch(true);
    font.setPointSize(10);

    NcCodeEditor::NcCodeEditorInstance()->setFont(font);
   
	connect(NcCodeEditor::NcCodeEditorInstance()->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));

	//adding the editor to the dockwidget
	mMainWindowUI->CodeWindow->setWidget(NcCodeEditor::NcCodeEditorInstance());
	mMainWindowUI->menuWindow->addAction(mMainWindowUI->CodeWindow->toggleViewAction());
}



void	NCMainWindow::setupSimulationSpeedController()
{
        QLabel *fasterLabel = new QLabel(tr("Fast"));
        QLabel *slowerLabel = new QLabel(tr("Slow"));

        mSpeedControllerSlider = new QSlider(Qt::Horizontal);
        
		mSpeedControllerSlider->setTickPosition(QSlider::TicksBelow);
        mSpeedControllerSlider->setMinimum(0);
        mSpeedControllerSlider->setMaximum(2000);
        mSpeedControllerSlider->setValue(1000);
        mSpeedControllerSlider->setTickInterval(200);

        QSize size = mSpeedControllerSlider->size();

        mSpeedControllerSlider->setMaximumSize(size.width() - 400, size.height());

		mMainWindowUI->toolBar_5->addWidget(slowerLabel);
        mMainWindowUI->toolBar_5->addWidget(mSpeedControllerSlider);
        mMainWindowUI->toolBar_5->addWidget(fasterLabel);
}

void	NCMainWindow:: setupVideoCaptureWindow()
{
	////adding the video streamer to the dockwidget
	//mMainWindowUI->videoWindow->setWidget(NcVideoCapture::NcVideoCaptureInstance());
	//mMainWindowUI->menuWindow->addAction(mMainWindowUI->videoWindow->toggleViewAction());
	//NcVideoCapture::NcVideoCaptureInstance()->startCapture();
}


void	NCMainWindow::documentWasModified()
{
	bool mod = NcCodeEditor::NcCodeEditorInstance()->document()->isModified();
    setWindowModified(NcCodeEditor::NcCodeEditorInstance()->document()->isModified());
}


void	NCMainWindow::toggleGrpahicsWindow()	//toggle graphics window
{
	/*if(mMainWindowUI->graphicsWindow->isVisible()==true)
		mMainWindowUI->graphicsWindow->setVisible(false);
	else
		mMainWindowUI->graphicsWindow->setVisible(true);*/
}


void	NCMainWindow::toggleNcCodeWindow()	//toggle Nc Code editor
{
	if(mMainWindowUI->CodeWindow->isVisible()==true)
		mMainWindowUI->CodeWindow->setVisible(false);
	else
		mMainWindowUI->CodeWindow->setVisible(true);
}

//for toggle the Tool Path window
void	NCMainWindow::toggleToolPathWindow()
{
	if(mMainWindowUI->ToolPathWindow->isVisible()==true)
		mMainWindowUI->ToolPathWindow->setVisible(false);
	else
		mMainWindowUI->ToolPathWindow->setVisible(true);
}


//for toggle the Property window 
void	NCMainWindow::togglePropertyWindow()
{
	if(mMainWindowUI->propertyWindow->isVisible()==true)
		mMainWindowUI->propertyWindow->setVisible(false);
	else
		mMainWindowUI->propertyWindow->setVisible(true);
}


//for toggle the tool window 
void	NCMainWindow::toggleToolWindow()
{
	if(mMainWindowUI->toolWindow->isVisible()==true)
		mMainWindowUI->toolWindow->setVisible(false);
	else
		mMainWindowUI->toolWindow->setVisible(true);
}





void	NCMainWindow::changeSimulationSpeed(int interval)
{
	mUpdateTimer->setInterval(mSpeedControllerSlider->maximum() - interval);
}


void	NCMainWindow::run()
{
	mMainWindowUI->actionRun->setEnabled(false);
	mMainWindowUI->actionPause->setEnabled(true);					
	connect(mMainWindowUI->actionPause, SIGNAL(triggered()), this, SLOT(pause()));

	NcSimulationController::getSimControllerInstance()->setSimulationState(RUNNING);

	mSimulationRunning = true;

	if(	NcSimulationController::getSimControllerInstance()->getSimulationMode() == BYCYCLE )
	{
		m_simulationWindow->setBoolForSimControl(true);
		connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(updateWidgets()));
		mUpdateTimer->setInterval(mSpeedControllerSlider->maximum() - mSpeedControllerSlider->value());
		mUpdateTimer->start();
	}	

	if(	NcSimulationController::getSimControllerInstance()->getSimulationMode() == BYBLOCK)
	{
		m_simulationWindow->setBoolForSimControl(true);
		connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(updateSimWinForBlockMode()));
		mUpdateTimer->setInterval(mSpeedControllerSlider->maximum() - mSpeedControllerSlider->value());
		mUpdateTimer->start();
	}
}

void	NCMainWindow::pause()
{
	mMainWindowUI->actionRun->setEnabled(true);
	mMainWindowUI->actionPause->setEnabled(false);

	//set simulation state to paused so that DL index sent to simulation window is the index at which 
	//simulation was stopped
	NcSimulationController::getSimControllerInstance()->setSimulationState(PAUSED);

	mUpdateTimer->stop();
	m_simulationWindow->pause();	
	
	mSimulationRunning = false;
	//write for pause in simulation controller
}

void	NCMainWindow::next()
{
	NcSimulationController::getSimControllerInstance()->setSimulationState(NEXT);
	NcSimulationController::getSimControllerInstance()->setOpIndexForNEXTState();

	mSimulationRunning = true;

	updateWidgets();
}




void	NCMainWindow::closeFile()
{
	//to change read write manager depending the current controller chosen
	if(NcFileReadWriteManager::getFileReadWriteInstance()->getCurrentNcFile() != 0)
    {
        /* excecuted when QFile(file) has been created i.e open case
           has been executed
        */

        if(isSaved())
        {
				QMessageBox::StandardButton ret;
				ret = QMessageBox::warning(this, tr("Application"),
										tr("Do you want to close?"),
										QMessageBox::Ok | QMessageBox::Cancel);

				if(ret == QMessageBox::Ok)
				{
			
					{
						cleanupReadWriteInstance cleanupReadWrite;
						cleanupNcMachineInstance cleanNcMachine;
						cleanupStaticWindowInstances cleanup;
						cleanupSimulationController cleanupSimController;
					}
		            
		                  
					mMainWindowUI->toolWindow->setVisible(false);
					mMainWindowUI->propertyWindow->setVisible(false);
					/*mMainWindowUI->graphicsWindow->setVisible(false);*/
					mMainWindowUI->CodeWindow->setVisible(false);
					mMainWindowUI->ToolPathWindow->setVisible(false);

					mMainWindowUI->action_Open->setEnabled(true);
					mMainWindowUI->action_New->setEnabled(true);

					mMainWindowUI->actionBackground->setEnabled(false);
					mMainWindowUI->actionUndo->setEnabled(false);
					mMainWindowUI->actionRedo->setEnabled(false);
					mMainWindowUI->actionCu_t->setEnabled(false);
					mMainWindowUI->actionFind->setEnabled(false);
					mMainWindowUI->action_Copy_2->setEnabled(false);
					mMainWindowUI->action_Paste_2->setEnabled(false);
					mMainWindowUI->action_Print->setEnabled(false);

					mMainWindowUI->actionRun->setEnabled(false);
					mMainWindowUI->actionPause->setEnabled(false);
					mMainWindowUI->actionNext->setEnabled(false);
				}
        }
    } 
    else
    {
        /* excecuted when QFile(file) has not been created
        i.e new case has been executed
        */
        if(isSaved())
		{
			centralWidget()->close();
            mMainWindowUI->CodeWindow->close();
            mMainWindowUI->ToolPathWindow->close();
            mMainWindowUI->toolWindow->close();
            mMainWindowUI->propertyWindow->close();
			//mMainWindowUI->videoWindow->close();
            mMainWindowUI->action_Open->setEnabled(true);
            mMainWindowUI->action_New->setEnabled(true);
            mMainWindowUI->actionRun->setEnabled(false);
            mMainWindowUI->actionPause->setEnabled(false);
            mMainWindowUI->actionNext->setEnabled(false);

            mMainWindowUI->actionBackground->setEnabled(false);
            mMainWindowUI->actionUndo->setEnabled(false);
            mMainWindowUI->actionRedo->setEnabled(false);
            mMainWindowUI->actionCu_t->setEnabled(false);
            mMainWindowUI->actionFind->setEnabled(false);
            mMainWindowUI->action_Copy_2->setEnabled(false);
            mMainWindowUI->action_Paste_2->setEnabled(false);
            mMainWindowUI->action_Print->setEnabled(false);
        }
    }

}


void NCMainWindow::closeEvent(QCloseEvent * ce)
{
	QString s = tr("Save changes ?");
	switch( QMessageBox::information(this, tr("DiscreteSimulator"), s, tr("Yes"), tr("No"),
				tr("Cancel"), 0, 2 ) )
	{
		case 0:
			/*if (!slotSaveProject())
			{
				ce->ignore();
				break;
			}*/
			writeSettings();//the recent projects must be saved
			ce->accept();
			break;

		case 1:
		default:
			writeSettings();//the recent projects must be saved
			ce->accept();
			break;

		case 2:
			ce->ignore();
			break;
	}
}


static const char *settingsGroup	= "MainWindow";
static const char *geometryKey		= "Geometry";
static const char *maxKey			= "Maximized";
static const char *fullScreenKey	= "FullScreen";
static const char *windowStateKey	= "windowState";

void NCMainWindow::readSettings()
{

	QSettings settings;
	settings.beginGroup(QLatin1String(settingsGroup));

	const QVariant geom = settings.value(QLatin1String(geometryKey));
	if (geom.isValid()) {
		restoreGeometry(settings.value(QLatin1String(geometryKey)).toByteArray());
	} else {
		resize(1024, 700);
	}
	restoreState(settings.value(QLatin1String(windowStateKey)).toByteArray());

	if (settings.value(QLatin1String(maxKey), false).toBool())
		setWindowState(Qt::WindowMaximized);

	settings.endGroup();

}

void NCMainWindow::writeSettings()
{
	QSettings settings;
	 settings.beginGroup(QLatin1String(settingsGroup));
    
    if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        settings.setValue(QLatin1String(maxKey), (bool) (windowState() & Qt::WindowMaximized));
        settings.setValue(QLatin1String(fullScreenKey), (bool) (windowState() & Qt::WindowFullScreen));
    } else {
        settings.setValue(QLatin1String(maxKey), false);
        settings.setValue(QLatin1String(fullScreenKey), false);
    }
	settings.setValue(QLatin1String(geometryKey), saveGeometry());
	settings.setValue(QLatin1String(windowStateKey), saveState());
    settings.endGroup();
}

void	NCMainWindow::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		m_simulationWindow->setBoolForSimControl(true);

		if( NcSimulationController::getSimControllerInstance()->getSimulationMode() == BYBLOCK  &&
			mSimulationRunning == true)
		{				
			mUpdateTimer->start();
			bybctrlbool = false;
			/*while(m_simulationWindow->updateSimulationWindow() != true);*/
		}

		/*m_simulationWindow->setBoolForSimControl(false);*/
	}
}

void	NCMainWindow::updateSimWinForBlockMode()
{
	if(bybctrlbool == false)
	{
		bybctrlbool = m_simulationWindow->updateSimulationWindow();
	}
	else
	{
		mUpdateTimer->stop();
		bybctrlbool = false;
		m_simulationWindow->setBoolForSimControl(false);
	}
}
