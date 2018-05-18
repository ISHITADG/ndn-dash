/*
 * qtsampleplayer.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include <QtWidgets>
#include <vector>
#include <sstream>
#include "QtSamplePlayerGui.h"
#include "IDASHPlayerGuiObserver.h"
#include "libdash.h"

using namespace sampleplayer;
using namespace sampleplayer::renderer;
using namespace dash::mpd;
using namespace libdash::framework::mpd;
using namespace libdash::framework::adaptation;

QtSamplePlayerGui::QtSamplePlayerGui    (QWidget *parent) : 
                										   QMainWindow          (parent),
														   ui                   (new Ui::QtSamplePlayerClass),
														   mpd                  (NULL),
														   fullscreen			(false),
														   isRunning			(false),
														   isPlaying			(false)
{
	InitializeCriticalSection(&(this->monitorMutex));
	this->ui->setupUi(this);
	this->SetVideoSegmentBufferFillState(0);
	this->SetVideoBufferFillState(0);
	this->SetAudioSegmentBufferFillState(0);
	this->SetAudioBufferFillState(0);
	this->SetAdaptationLogicComboBox(mpd,this->ui->cb_adaptationlogic);
	this->ui->button_stop->setEnabled(false);
	this->ui->button_start->setEnabled(false);
//	this->myStackedWidget = new QStackedWidget;
//	this->myStackedWidget->addWidget(this->ui->centralWidget);
//	this->setCentralWidget(this->myStackedWidget);
	this->setWindowFlags(Qt::Window);
}
QtSamplePlayerGui::~QtSamplePlayerGui   ()
{
	DeleteCriticalSection(&(this->monitorMutex));
	delete (this->ui);
}

void			QtSamplePlayerGui::EnableUserActions								()
{
	this->ui->cb_period->setEnabled(true);
	this->ui->cb_audio_adaptationset->setEnabled(true);
	this->ui->cb_video_adaptationset->setEnabled(true);
	this->ui->cb_audio_representation->setEnabled(true);
	this->ui->cb_video_representation->setEnabled(true);
	this->Reset();
}
void			QtSamplePlayerGui::DisableUserActions								()
{
	this->ui->cb_period->setEnabled(false);
	this->ui->cb_audio_adaptationset->setEnabled(false);
	this->ui->cb_video_adaptationset->setEnabled(false);
	this->ui->cb_audio_representation->setEnabled(false);
	this->ui->cb_video_representation->setEnabled(false);
}

void		QtSamplePlayerGui::Reset												()
{
	this->ui->button_start->setEnabled(true);
	this->ui->button_stop->setEnabled(false);
	this->ui->cb_mpd->setDisabled(false);
	this->ui->lineEdit_mpd->setDisabled(false);
	this->ui->button_mpd->setDisabled(false);
}

void            QtSamplePlayerGui::ClearComboBoxes                                  ()
{
	this->ui->cb_period->clear();

	this->ui->cb_video_adaptationset->clear();
	this->ui->cb_video_representation->clear();

	this->ui->cb_audio_adaptationset->clear();
	this->ui->cb_audio_representation->clear();
}
QTGLRenderer*   QtSamplePlayerGui::GetVideoElement                                  ()
{
	return this->ui->videoelement;
}

void            QtSamplePlayerGui::SetGuiFields                                     (dash::mpd::IMPD* mpd)
{
	this->LockUI();
	this->ClearComboBoxes();
	this->SetPeriodComboBox(mpd, this->ui->cb_period);
	if (mpd->GetPeriods().size() > 0)
	{
		IPeriod *period = mpd->GetPeriods().at(0);

		this->SetVideoAdaptationSetComboBox(period, this->ui->cb_video_adaptationset);
		this->SetAudioAdaptationSetComboBox(period, this->ui->cb_audio_adaptationset);

		if (!AdaptationSetHelper::GetVideoAdaptationSets(period).empty())
		{
			IAdaptationSet *adaptationSet = AdaptationSetHelper::GetVideoAdaptationSets(period).at(0);

			this->SetRepresentationComoboBox(adaptationSet, this->ui->cb_video_representation);
		}
		if (!AdaptationSetHelper::GetAudioAdaptationSets(period).empty())
		{
			IAdaptationSet *adaptationSet = AdaptationSetHelper::GetAudioAdaptationSets(period).at(0);

			this->SetRepresentationComoboBox(adaptationSet, this->ui->cb_audio_representation);
		}
	}

	this->mpd = mpd;
	this->UnLockUI();

	this->ui->button_start->setEnabled(true);
}

bool QtSamplePlayerGui::GetNDNStatus			()
{
	return this->ui->ndnCheckBox->isChecked();
}

void QtSamplePlayerGui::SetNDNStatus			(bool value)
{
	this->ui->ndnCheckBox->setCheckState(value? Qt::Checked : Qt::Unchecked);
}
//LogicType
int QtSamplePlayerGui::GetAdaptationLogic     	()
{
	return this->ui->cb_adaptationlogic->currentIndex();
}
void            QtSamplePlayerGui::SetVideoSegmentBufferFillState                   (int percentage)
{
	EnterCriticalSection(&(this->monitorMutex));
	this->ui->progressBar_V->setValue(percentage);
	LeaveCriticalSection(&(this->monitorMutex));
}
void            QtSamplePlayerGui::SetVideoBufferFillState                          (int percentage)
{
	EnterCriticalSection(&(this->monitorMutex));
	this->ui->progressBar_VF->setValue(percentage);
	LeaveCriticalSection(&(this->monitorMutex));
}
void            QtSamplePlayerGui::SetAudioSegmentBufferFillState                   (int percentage)
{
	EnterCriticalSection(&(this->monitorMutex));
	this->ui->progressBar_A->setValue(percentage);
	LeaveCriticalSection(&(this->monitorMutex));
}
void            QtSamplePlayerGui::SetAudioBufferFillState                          (int percentage)
{
	EnterCriticalSection(&(this->monitorMutex));
	this->ui->progressBar_AC->setValue(percentage);
	LeaveCriticalSection(&(this->monitorMutex));
}
void            QtSamplePlayerGui::AddWidgetObserver                                (IDASHPlayerGuiObserver *observer)
{
	this->observers.push_back(observer);
}
void            QtSamplePlayerGui::SetStatusBar                                     (const std::string& text)
{
	QString str(text.c_str());
	this->ui->statusBar->showMessage(str);
}
void            QtSamplePlayerGui::SetRepresentationComoboBox                       (dash::mpd::IAdaptationSet *adaptationSet, QComboBox *cb)
{
	std::vector<IRepresentation *> represenations = adaptationSet->GetRepresentation();
	cb->clear();

	for(size_t i = 0; i < represenations.size(); i++)
	{
		IRepresentation *representation = represenations.at(i);

		std::stringstream ss;
		ss << representation->GetId() << " " << representation->GetBandwidth() << " bps "  << representation->GetWidth() << "x" << representation->GetHeight();

		cb->addItem(QString(ss.str().c_str()));
	}
}
void            QtSamplePlayerGui::SetAdaptationSetComboBox                         (dash::mpd::IPeriod *period, QComboBox *cb)
{
	std::vector<IAdaptationSet *> adaptationSets = period->GetAdaptationSets();
	cb->clear();

	for(size_t i = 0; i < adaptationSets.size(); i++)
	{
		IAdaptationSet *adaptationSet = adaptationSets.at(i);

		std::stringstream ss;
		ss << "AdaptationSet " << i+1;

		cb->addItem(QString(ss.str().c_str()));
	}
}
void            QtSamplePlayerGui::SetAudioAdaptationSetComboBox                    (dash::mpd::IPeriod *period, QComboBox *cb)
{
	std::vector<IAdaptationSet *> adaptationSets = AdaptationSetHelper::GetAudioAdaptationSets(period);
	cb->clear();

	for(size_t i = 0; i < adaptationSets.size(); i++)
	{
		IAdaptationSet *adaptationSet = adaptationSets.at(i);

		std::stringstream ss;
		ss << "AdaptationSet " << i+1;

		cb->addItem(QString(ss.str().c_str()));
	}
}
void            QtSamplePlayerGui::SetVideoAdaptationSetComboBox                    (dash::mpd::IPeriod *period, QComboBox *cb)
{
	std::vector<IAdaptationSet *> adaptationSets = AdaptationSetHelper::GetVideoAdaptationSets(period);
	cb->clear();

	for(size_t i = 0; i < adaptationSets.size(); i++)
	{
		IAdaptationSet *adaptationSet = adaptationSets.at(i);

		std::stringstream ss;
		ss << "AdaptationSet " << i+1;

		cb->addItem(QString(ss.str().c_str()));
	}
}
void            QtSamplePlayerGui::SetPeriodComboBox                                (dash::mpd::IMPD *mpd, QComboBox *cb)
{
	std::vector<IPeriod *> periods = mpd->GetPeriods();
	cb->clear();

	for(size_t i = 0; i < periods.size(); i++)
	{
		IPeriod *period = periods.at(i);

		std::stringstream ss;
		ss << "Period " << i+1;

		cb->addItem(QString(ss.str().c_str()));
	}
}

void            QtSamplePlayerGui::SetAdaptationLogicComboBox (dash::mpd::IMPD *mpd, QComboBox *cb)
{
	cb->clear();

	for(int i = 0; i < Count; i++)
	{
		cb->addItem(QString(LogicType_string[i]));
	}
}

void            QtSamplePlayerGui::LockUI                                           ()
{
	this->setEnabled(false);
}
void            QtSamplePlayerGui::UnLockUI                                         ()
{
	this->setEnabled(true);
}
std::string     QtSamplePlayerGui::GetUrl                                           ()
{
	this->LockUI();

	std::string ret  = this->ui->lineEdit_mpd->text().toStdString();

	this->UnLockUI();
	return ret;
}

/* Notifiers */
void            QtSamplePlayerGui::NotifySettingsChanged                            ()
{
	this->LockUI();

	int period              = this->ui->cb_period->currentIndex();
	int videoAdaptionSet    = this->ui->cb_video_adaptationset->currentIndex();
	int videoRepresentation = this->ui->cb_video_representation->currentIndex();
	int audioAdaptionSet    = this->ui->cb_audio_adaptationset->currentIndex();
	int audioRepresentation = this->ui->cb_audio_representation->currentIndex();

	for(size_t i = 0; i < this->observers.size(); i++)
		this->observers.at(i)->OnSettingsChanged(period, videoAdaptionSet, videoRepresentation, audioAdaptionSet, audioRepresentation);

	this->UnLockUI();
}
void            QtSamplePlayerGui::NotifyMPDDownloadPressed                         (const std::string &url)
{
	for(size_t i = 0; i < this->observers.size(); i++)
		this->observers.at(i)->OnDownloadMPDPressed(url);
}
void            QtSamplePlayerGui::NotifyStartButtonPressed                         ()
{
	this->LockUI();

	int period              = this->ui->cb_period->currentIndex();
	int videoAdaptionSet    = this->ui->cb_video_adaptationset->currentIndex();
	int videoRepresentation = this->ui->cb_video_representation->currentIndex();
	int audioAdaptionSet    = this->ui->cb_audio_adaptationset->currentIndex();
	int audioRepresentation = this->ui->cb_audio_representation->currentIndex();

	for(size_t i = 0; i < this->observers.size(); i++)
		this->observers.at(i)->OnStartButtonPressed(period, videoAdaptionSet, videoRepresentation, audioAdaptionSet, audioRepresentation);
	this->isPlaying = true;
	this->UnLockUI();
}
void			QtSamplePlayerGui::NotifyPauseButtonPressed							()
{
	for(size_t i = 0; i < this->observers.size(); i++)
			this->observers.at(i)->OnPauseButtonPressed();
		this->isPlaying = !this->isPlaying;

}
void			QtSamplePlayerGui::NotifyFastForward								()
{
	for(size_t i = 0; i < this->observers.size(); i++)
			this->observers.at(i)->OnFastForward();
		this->isPlaying = !this->isPlaying;

}
void			QtSamplePlayerGui::NotifyFastRewind									()
{
	for(size_t i = 0; i < this->observers.size(); i++)
			this->observers.at(i)->OnFastRewind();
		this->isPlaying = !this->isPlaying;

}
void            QtSamplePlayerGui::NotifyStopButtonPressed                          ()
{
	for(size_t i = 0; i < this->observers.size(); i++)
		this->observers.at(i)->OnStopButtonPressed();
}

/* UI Slots */
void            QtSamplePlayerGui::on_button_mpd_clicked                            ()
{
	this->mpd = NULL;
	this->NotifyMPDDownloadPressed(this->GetUrl());
}
void            QtSamplePlayerGui::on_cb_period_currentIndexChanged                 (int index)
{
	if(index == -1 || this->mpd == NULL)
		return; // No Item set

	this->LockUI();

	this->SetAudioAdaptationSetComboBox(mpd->GetPeriods().at(index), ui->cb_audio_adaptationset);
	this->SetVideoAdaptationSetComboBox(mpd->GetPeriods().at(index), ui->cb_video_adaptationset);

	this->NotifySettingsChanged();

	this->UnLockUI();
}
void            QtSamplePlayerGui::on_cb_mpd_currentTextChanged                     (const QString &arg1)
{
	this->ui->button_start->setDisabled(true);
	this->ui->lineEdit_mpd->setText(arg1);
}

void			QtSamplePlayerGui::SetUrl											(const char * text)
{
	this->ui->lineEdit_mpd->setText(QString(text));
}

void			QtSamplePlayerGui::SetAdaptationLogic				(LogicType type)
{
	this->ui->cb_adaptationlogic->setCurrentIndex((int) type);
}
void            QtSamplePlayerGui::on_cb_video_adaptationset_currentIndexChanged    (int index)
{
	if(index == -1 || this->mpd == NULL)
		return; // No Item set

	this->LockUI();

	IPeriod *period = this->mpd->GetPeriods().at(this->ui->cb_period->currentIndex());

	this->SetRepresentationComoboBox(AdaptationSetHelper::GetVideoAdaptationSets(period).at(index), this->ui->cb_video_representation);

	this->NotifySettingsChanged();

	this->UnLockUI();
}
void            QtSamplePlayerGui::on_cb_video_representation_currentIndexChanged   (int index)
{
	if(index == -1)
		return; // No Item set

	this->NotifySettingsChanged();
}
void            QtSamplePlayerGui::on_cb_audio_adaptationset_currentIndexChanged    (int index)
{
	if(index == -1 || this->mpd == NULL)
		return; // No Item set

	this->LockUI();

	IPeriod *period = this->mpd->GetPeriods().at(this->ui->cb_period->currentIndex());

	this->SetRepresentationComoboBox(AdaptationSetHelper::GetAudioAdaptationSets(period).at(index), this->ui->cb_audio_representation);

	this->NotifySettingsChanged();

	this->UnLockUI();
}
void            QtSamplePlayerGui::on_cb_audio_representation_currentIndexChanged   (int index)
{
	if(index == -1)
		return; // No Item set

	this->NotifySettingsChanged();
}
void			QtSamplePlayerGui::LockStartAndDownloadButton						()
{
	this->ui->button_start->setEnabled(false);
	this->ui->button_stop->setEnabled(true);
	this->ui->cb_mpd->setDisabled(true);
	this->ui->lineEdit_mpd->setDisabled(true);
	this->ui->button_mpd->setDisabled(true);
}
void            QtSamplePlayerGui::on_button_start_clicked                          ()
{
	this->ui->button_start->setEnabled(false);
	this->ui->button_stop->setEnabled(true);
	this->ui->cb_mpd->setDisabled(true);
	this->ui->lineEdit_mpd->setDisabled(true);
	this->ui->button_mpd->setDisabled(true);

	this->NotifyStartButtonPressed();
}
void            QtSamplePlayerGui::on_button_stop_clicked                           ()
{
	this->ui->button_start->setEnabled(true);
	this->ui->button_stop->setEnabled(false);
	this->ui->cb_mpd->setDisabled(false);
	this->ui->lineEdit_mpd->setDisabled(false);
	this->ui->button_mpd->setDisabled(false);

	this->NotifyStopButtonPressed();
}

void			QtSamplePlayerGui::mouseDoubleClickEvent							(QMouseEvent* event)
{
	this->OnDoubleClick();
}

void			QtSamplePlayerGui::OnDoubleClick									()
{
	FullScreenDialog *dlg = new FullScreenDialog(this);
	QHBoxLayout *dlgLayout = new QHBoxLayout(dlg);
	dlgLayout->setContentsMargins(0,0,0,0);
	dlgLayout->addWidget(this->ui->videoelement);
	dlg->setLayout(dlgLayout);
	dlg->setWindowFlags(Qt::Window);
	dlg->showFullScreen();

	bool r = connect(dlg, SIGNAL(rejected()), this, SLOT(showNormal()));
	assert(r);
	r = connect(dlg, SIGNAL(accepted()), this, SLOT(showNormal()));
	assert(r);
}

void 			QtSamplePlayerGui::showNormal()
{
	this->ui->verticalLayout_3->addWidget(this->ui->videoelement);
}
/*void			QtSamplePlayerGui::OnDoubleClick									()
{
	if(!this->isRunning)
	{
		return;
	}
	if(this->fullscreen)
	{
		this->showNormal();
		this->myStackedWidget->setCurrentWidget(this->ui->centralWidget);
		this->myStackedWidget->removeWidget(this->ui->videoelement);
		this->ui->verticalLayout_3->addWidget(this->ui->videoelement);
		this->ui->videoelement->show();
		this->fullscreen = false;
	}
	else
	{
		this->ui->statusBar->hide();
		this->myStackedWidget->addWidget(this->ui->videoelement);
		this->myStackedWidget->setCurrentWidget(this->ui->videoelement);
		this->showFullScreen();
		this->fullscreen = true;
	}
} */


void QtSamplePlayerGui::keyPressEvent												(QKeyEvent* event)
{
	switch(event->key())
	{
			//Play/Pause
			case Qt::Key_P:
			{
				this->NotifyPauseButtonPressed();
				break;
			}
			//Full Screen
			case Qt::Key_F:
			{
				this->OnDoubleClick();
				break;
			}
			//Fast Forward
			case Qt::Key_L:
			{
				this->NotifyFastForward();
				break;
			}
			//Fast Rewind
			case Qt::Key_T:
			{
				this->NotifyFastRewind();
				break;
			}
			default:
			{
				break;
			}
	}
}
