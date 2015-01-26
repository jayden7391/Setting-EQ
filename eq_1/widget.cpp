#include <QMessageBox>
#include <QFileDialog>
#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    channel(0)
{
    ui->setupUi(this);
    ui->equalizerLayout->addWidget(&visualEq);

    
    if (!BASS_Init(-1, 44100, 0, 0, 0)) {
        QMessageBox::critical(this, tr("Visualizer"),
            tr("Can't init audio!"), QMessageBox::Ok);
    }
 
    currentMode = Widget::Stop;

    ui->slider->setEnabled(false);

    connect(ui->openFileBtn, SIGNAL(clicked()), this, SLOT(openFileSlot()));
    connect(ui->playFileBtn, SIGNAL(clicked()), this, SLOT(playFileSlot()));
    connect(ui->stopFileBtn, SIGNAL(clicked()), this, SLOT(stopFileSlot()));
    connect(ui->pauseFileBtn, SIGNAL(clicked()), this, SLOT(pauseFileSlot()));
    connect(ui->slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMovedSlot(int)));
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

Widget::~Widget()
{
    delete ui;
    
    BASS_Stop();              
    BASS_StreamFree(channel); 
    BASS_Free();              
}

void Widget::openFileSlot()
{
    stopFileSlot();
    BASS_StreamFree(channel);

    ui->slider->setEnabled(false);

    filename = QFileDialog::getOpenFileName(this,
        "Open audio file", QDir::currentPath(), "Audio files (*.wav *.mp3)");
    ui->openedFileLabel->setText(QString("Opened: ")+filename);

    
    channel = BASS_StreamCreateFile(FALSE, filename.toLocal8Bit().data(), 0, 0, 0);

    
    if (channel == 0) {
        QMessageBox::warning(this, "Visualizer",
            "Can't load '"+filename+"'!", QMessageBox::Ok);
        return;
    }

    
    ui->slider->setEnabled(true);
    ui->slider->setMinimum(0);
    ui->slider->setMaximum(BASS_ChannelGetLength(channel, 0) - 1);
}

void Widget::playFileSlot()
{
    if (currentMode == Widget::Play)
        return;

    
    QFile file(filename);

    if (!file.exists())
    {
        QMessageBox::warning(this, "Visualizer",
            "File '"+filename+"' does not exist!", QMessageBox::Ok);

        
        BASS_ChannelStop(channel);
        BASS_StreamFree(channel);
        return;
    }

    
    if (!BASS_ChannelPlay(channel, FALSE))
    {
        QMessageBox::warning(this, "Visualizer",
            "Can't to play '"+filename+"'!", QMessageBox::Ok);
        return;
    }

    
    currentMode = Widget::Play;

    
    timer.start(30);
}

void Widget::pauseFileSlot()
{
    if (currentMode == Widget::Play) {
        BASS_ChannelPause(channel);   
        currentMode = Widget::Pause;  
        visualEq.clearScene();        
        visualEq.update();
    }
}

void Widget::stopFileSlot()
{
    BASS_ChannelStop(channel);   
    currentMode = Widget::Stop;  
    timer.stop();
    ui->slider->setSliderPosition(0); 
    visualEq.clearScene();            
    visualEq.update();
}

void Widget::sliderMovedSlot(int val)
{
  BASS_ChannelSetPosition(channel, val, 0);
}

void Widget::timerSlot()
{
    unsigned int currentPos = BASS_ChannelGetPosition(channel, 0);
    if (currentPos == BASS_ChannelGetLength(channel, 0))
    {
        stopFileSlot();
        return;
    }

    ui->slider->setSliderPosition(currentPos);

    
    BASS_ChannelGetData(channel, visualEq.fftData, BASS_DATA_FFT256);
    visualEq.update();
}
