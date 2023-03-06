#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QApplication>
#include <QDialog>
#include <QAudioInput>
#include <QAudioOutput>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QtNetwork>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QString>
#include <QBuffer>
#include <QInputDialog>
#include <wav_header.h>
#include <stdio.h>
#include <string.h>


#define VOLUME       (1.0)       //0.0 to 1.0
//https://www.kozco.com/tech/soundtests.html

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
  {
    ui->setupUi(this);
    setWindowTitle("Wifi-mic Player");

    ui->Volume_snd->setToolTip("Sound volume");
    ui->Volume_snd->setToolTipDuration(3000);
    ui->Volume_snd->setValue(sound_volume_setting(-1)*100);
    //updateRecentServers();
    QString tmp = "";
    adjustForCurrentServers(tmp);
    ui->url_server->addItem("RESET LIST");
    ui->url_server->setToolTip("http://login:pass@ip:port/rec.wav");
#ifdef __ANDROID__
    ui->url_server->adjustSize();
    ui->url_server->resize(ui->url_server->width(), ui->url_server->height() + 30);
    ui->playButton->setGeometry(QRect(40, 200, 300, 100));
    ui->quitButton->setGeometry(QRect(700, 200, 300, 100));
    //ui->playButton->adjustSize();
    ui->Volume_snd->setGeometry(QRect(250,200, 200, 30));
    ui->Volume_snd->hide();
    ui->progressBar->setGeometry(QRect(40, 400, 1000, 20));
    ui->time_lbl->setGeometry(QRect(500, 500, 100, 20));
    ui->time_lbl->setText("00:00");
    ui->time_lbl->adjustSize();
    connect(ui->quitButton, &QPushButton::clicked, this, &QDialog::close);
#else
    ui->quitButton->hide();
#endif
    ui->url_server->setToolTipDuration(3000);

    networkManager = new QNetworkAccessManager(this);

    ui->statusBar->setStyleSheet("color: gray; text-align: center");

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::play);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::replyFinished);
    connect(networkManager, &QNetworkAccessManager::authenticationRequired, this, &MainWindow::onAuthenticationRequest);

  }

MainWindow::~MainWindow()
  {
      delete ui;
  }

void MainWindow::on_Volume_snd_valueChanged(int value){
#ifndef __ANDROID__
    if (audioOutput) audioOutput->setVolume((qreal)value/100);
    sound_volume_setting((qreal)value/100);
    qDebug() << "setVolume ->" << (qreal)value/100;
#else
    (void) value;
#endif
}


void MainWindow::play() {

    if (!play_started) {
        byteArray.resize(500000);
        ui->progressBar->setMaximum(0);
        QString temp = ui->url_server->currentText();
        adjustForCurrentServers(temp);
        play_started = true;
        ui->playButton->setText("STOP");
        ui->playButton->setStyleSheet("color: red");

        url_str = ui->url_server->currentText();
        QString login_pass;
        if (url_str.contains("http://") || url_str.contains("https://")){
        //    if (url_str.count(":") == 3 && url_str.count("@") == 1){
             if (url_str.count(":") == 3){
               login_pass = url_str.mid(7, url_str.indexOf("@") - 7);
               qDebug() << " login_pass" << login_pass;
            }
        }
        else {
            ui->statusBar->showMessage("wrong url", 2000);
            play();  //emulate stop
            return;
        }
        QNetworkRequest request(QUrl(ui->url_server->currentText()));
        QByteArray data;
        //data.append("Basic " + QByteArray("admin:admin").toBase64());
        data.append("Basic " + QByteArray(login_pass.toUtf8()).toBase64());
        request.setRawHeader("Authorization", data);
        request.setRawHeader("Accept", "*/*");
        request.setRawHeader("Range", "bytes=0-");
        request.setRawHeader("Accept-Encoding", QByteArray("*"));
        //request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/x-raw");
        ui->statusBar->showMessage("connecting to server", 2000);
        reply_ = networkManager->get(request);
        reply_->setReadBufferSize(TCP_READ_BUFF_SIZE);
        connect(reply_, &QNetworkReply::finished, this,  &MainWindow::httpFinished);
        connect(reply_, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);
        qDebug() << " PLAY";
        ui->url_server->setEditable(false);
    }
    else {
        qDebug() << " STOP";
        play_started = false;
        ui->playButton->setText("PLAY");
        ui->playButton->setStyleSheet("color: black");
        ui->url_server->setEditable(true);
        posic = 0;
        started = false;

        if (audioOutput && audioOutput->state() == QAudio::State::ActiveState) {
            reply_->abort();
            audioOutput->stop();
            audioBuffer.seek(audioBuffer.size());

            disconnect(reply_, &QNetworkReply::finished, this,  &MainWindow::httpFinished);
            disconnect(reply_, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);

        }
        if (audioOutput) disconnect(audioOutput, &QAudioOutput::notify, this, &MainWindow::handleNotify);
        QDateTime thisDT = QDateTime::fromTime_t(0);
        ui->time_lbl->setText(thisDT.toString("mm:ss"));
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(6000);
    }
}



void MainWindow::httpReadyRead() {
    if (reply_->error()) {
        qDebug() << "Error:" << reply_->errorString();
        return;
    }
    qDebug() << " httpReadyRead aviable = " << QString::asprintf("%6lld", reply_->bytesAvailable());// << " audioBuffer->pos() = " << QString::asprintf("%8lld", audioBuffer.pos());

    if (posic >= 16384*4 && !started) {  //16384*3
        memcpy(wav_hdr, byteArray.data(), 44);
        if (processing_wav_header(wav_hdr)) { //not valid wav file
            play();  //emulate press STOP
            return;
        }
        if (play_time_sec != 0){
            ui->progressBar->setMaximum(play_time_sec);
        }
        started = true;
        audioBuffer.seek(44);
        audioOutput->start(&audioBuffer);
        ui->statusBar->showMessage("playing media", 2000);
        int_buf_sze = audioOutput->bufferSize();
        qDebug() << " audioOutput->setBufferSize2 ->" << int_buf_sze;
        disconnect(reply_, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);


    }
    QByteArray data;
    int32_t sze;
    sze = reply_->bytesAvailable();
    if ((posic + sze) > byteArray.size()) {
        sze = byteArray.size() - posic;
    }
    data = reply_->read(sze);
    byteArray.replace(posic, sze, data);
    posic+= sze;
    if (posic >= byteArray.size()) {
        posic = 0;
    }

    if (started) {
        if (posic > audioBuffer.pos()) delta = posic - audioBuffer.pos();
        else delta = (byteArray.size() + posic) - audioBuffer.pos();
        if (audioOutput->state() == QAudio::State::SuspendedState && delta > int_buf_sze*3) {
            audioOutput->resume();  //buffer full, continue audio
            disconnect(reply_, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);
        }
    }
}

  void MainWindow::httpFinished() {
      if (reply_->error()) {
                  ui->statusBar->showMessage(reply_->errorString(), 2000);
                  qDebug() << " networkManager warning:" << reply_->errorString();
                  if (reply_->errorString().contains("Host requires authentication") || reply_->errorString().contains("not found") || reply_->errorString().contains("is unknown")) {
                    play();  //emulate stop
                  }
                  return;
              }
      qDebug() << " httpFinished()";

    }

  void MainWindow::replyFinished(QNetworkReply *reply) {
      if (reply->error()) {
                  ui->statusBar->showMessage(reply->errorString(), 2000);
                  qDebug() << " reply_ warning:" << reply->errorString();
                  return;
              }

      qDebug() << " replyFinished()";

    }

  void MainWindow::onAuthenticationRequest(QNetworkReply *,QAuthenticator *aAuthenticator){

      bool ok;
      QString text = QInputDialog::getText(this, tr("Enter username and password separated by a colon"),
                                           tr("login:pass"), QLineEdit::Normal,
                                           "", &ok);
      if (ok && !text.isEmpty())  {
          aAuthenticator->setUser(text.mid(0, text.indexOf(":") - 0));
          aAuthenticator->setPassword(text.mid(text.indexOf(":") + 1, -1));
      }
      qDebug() << " onAuthenticationRequest()" << " login:pass" << text.mid(0, text.indexOf(":") - 0) << ":" << text.mid(text.indexOf(":") + 1, -1);
  }

  int MainWindow::processing_wav_header( char *header_ptr){

      WAVRIFFHEADER *header   = (WAVRIFFHEADER *) header_ptr;
      FORMATCHUNK *format_wav = (FORMATCHUNK *) (header_ptr + 12);
      DATACHUNK   *data_wav   = (DATACHUNK *) (header_ptr + 36);
      // Check for valid header
      if( strncmp(header->groupID, "RIFF", 4) != 0 || strncmp(header->riffType, "WAVE", 4) != 0 )
      {
          qDebug() <<  "File is not a valid wave file 1." << QString(header->groupID)  <<"::" << QString(header->riffType);
          return 1;
      }
      else{
          qDebug() <<  " format_wav->chunkID" << QString::fromUtf8(format_wav->chunkID, 3);
          if( strncmp(format_wav->chunkID, "fmt", 3) == 0){
              if( ((format_wav->wBitsPerSample != 8) && (format_wav->wBitsPerSample != 16) && (format_wav->wBitsPerSample != 32)) || (format_wav->chunkSize % (int)format_wav->wBlockAlign != 0) )
              {
                  qDebug() <<  "File is not a valid wave file 2." ;
                  return 1;
              }
              if( strncmp(data_wav->chunkID, "data", 4) == 0){
                  format.setSampleRate(format_wav->dwSamplesPerSec); qDebug() << " format.setSampleRate" << format_wav->dwSamplesPerSec;
                  format.setChannelCount(format_wav->wChannels);     qDebug() << " format.setChannelCount" << format_wav->wChannels;
                  format.setSampleSize(format_wav->wBitsPerSample);  qDebug() << " format.setSampleSize" << format_wav->wBitsPerSample;
                  format.setCodec("audio/pcm");
                  format.setByteOrder(QAudioFormat::LittleEndian);
                  format.setSampleType(QAudioFormat::SignedInt);
                  QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
                  if (!info.isFormatSupported(format)) {
                      qWarning() << "Raw audio format not supported by backend, cannot play audio.";
                      return 1;
                  }
                  //play_size = smp_rate * bits_per_sample/8 * PLAY_TIME;
                  qDebug() << " data_wav->size ->" << data_wav->chunkSize;
                  play_time_coef = format_wav->dwSamplesPerSec * format_wav->wBitsPerSample/8;
                  play_time_sec = data_wav->chunkSize / (format_wav->dwSamplesPerSec * format_wav->wChannels * format_wav->wBitsPerSample/8);
                  qDebug() << " play_time_sec ->" << play_time_sec;
                  byteArray.resize(10 * ((format_wav->dwSamplesPerSec * format_wav->wChannels * format_wav->wBitsPerSample/8))); //for 10 sec  ///////////////////////////
                  audioBuffer.setBuffer(&byteArray);
                  audioBuffer.open(QBuffer::ReadWrite);
                  audioOutput = new QAudioOutput(format, this);
                  audioOutput->setNotifyInterval(50);
                  connect(audioOutput, &QAudioOutput::stateChanged, this, &MainWindow::handleStateChanged);
                  connect(audioOutput, &QAudioOutput::notify, this, &MainWindow::handleNotify);
                  quint64 tmp = ((format_wav->dwSamplesPerSec * format_wav->wChannels * format_wav->wBitsPerSample/8) * SIZE_AUDIO_BUF_IN_SEC);
                  audioOutput->setBufferSize(tmp); //////////////////
                  qDebug() << " audioOutput->setBufferSize1 ->" << tmp;

#ifdef __ANDROID__
                  audioOutput->setVolume(0.99);
#else
                  audioOutput->setVolume((qreal)ui->Volume_snd->value()/100);
#endif

              }
          }
      }
      return 0;
  }

  void MainWindow::handleStateChanged(QAudio::State state) {
      qWarning() << " audioOutput state -> " << state;
      switch (state) {
      case QAudio::IdleState:
              if (play_started) play();
              audioOutput->stop();
              audioBuffer.close();
              byteArray.clear();
              disconnect(audioOutput, &QAudioOutput::stateChanged, this, &MainWindow::handleStateChanged);
              posic = 0;
              delete audioOutput;
              audioOutput = nullptr;
          break;

      case QAudio::ActiveState:

          //qDebug() << " QAudio::ActiveState";
          break;

      case QAudio::StoppedState:
          // Stopped for other reasons
          if (audioOutput->error() != QAudio::NoError) {
              // Error handling
              qDebug() << " audioOutput->error()" << audioOutput->error();
          }
          //qApp->exit();
          break;

      default:
          // ... other cases as appropriate
          break;
      }
  }

  void MainWindow::adjustForCurrentServers(QString &server_str){

      QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
      QSettings settings("recent_srv.ini", QSettings::IniFormat);
      //QString appPath = qApp->applicationFilePath();
      QStringList recentServers =
              settings.value("recentServers").toStringList();
      recentServers.removeAll(server_str);
      if (server_str.size()) recentServers.prepend(server_str);
      while (recentServers.size() > maxSrvNr)
            recentServers.removeLast();
      if (!settings.isWritable() || settings.status() != QSettings::NoError)
          qDebug() << " error ->" << settings.status();
      if (recentServers.size() > 0) {
          settings.setValue("recentServers", recentServers);
      }
      else {  //if the file is empty
          qDebug() << recentServers.size();
          recentServers.append("http://admin:admin@wifi-mic.local:8088/rec.wav");
          recentServers.append("https://www.kozco.com/tech/LRMonoPhase4.wav");
          recentServers.append("https://www.kozco.com/tech/piano2.wav");
          recentServers.append("https://www.kozco.com/tech/c304-2.wav");
          recentServers.append("https://www.kozco.com/tech/WAV-MP3.wav");
          recentServers.append("https://www.kozco.com/tech/audio/pulse-17.wav");
          settings.setValue("recentServers", recentServers);
          qDebug() << " fill defaults servers";
      }

      settings.sync();

      updateRecentServers();
  }

  void MainWindow::updateRecentServers(){

      QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
      QSettings settings("recent_srv.ini", QSettings::IniFormat);
      QStringList recentServers_ =
              settings.value("recentServers").toStringList();

      auto itEnd = 0u;
      if(recentServers_.size() <= maxSrvNr)
          itEnd = recentServers_.size();
      else
          itEnd = maxSrvNr;

      ui->url_server->clear();
      for (auto i = 0u; i < itEnd; ++i) {
          ui->url_server->addItem(recentServers_.at(i));
          qDebug() << " comboBox->addItem ->" << recentServers_.at(i);
      }

      if (recentServers_.size() == 0) {
 //         ui->url_server->addItem("http://admin:admin@wifi-mic.local:8088/rec.wav");
          qDebug() << " comboBox->addItem ->" <<  ui->url_server->currentText();
      }

  }

  qreal MainWindow::sound_volume_setting(qreal snd_vol){

      QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
      QSettings settings("recent_srv.ini", QSettings::IniFormat);

      qreal s_vol = settings.value("sound_volume").toReal();

      if (!settings.isWritable() || settings.status() != QSettings::NoError)
          qDebug() << " error ->" << settings.status();
      if (snd_vol >= 0) {   //write mode
          settings.setValue("sound_volume", snd_vol);
          settings.sync();
          return snd_vol;
      }
      else return s_vol;    //read mode

  }

  void MainWindow::handleNotify() {

      QDateTime thisDT = QDateTime::fromTime_t(audioOutput->processedUSecs()/1000000);
      ui->time_lbl->setText(thisDT.toString("mm:ss"));
      ui->progressBar->setValue(audioOutput->processedUSecs()/1000000);

      //audio has ended
      if (audioOutput->processedUSecs()/1000000 >= play_time_sec) {
          byteArray.truncate(posic);
          return;
      }

      if (audioBuffer.pos() == audioBuffer.size()) {
          audioBuffer.seek(0);
          qDebug() << " audioBuffer.pos -> " << audioBuffer.pos();
      }

      if (posic > audioBuffer.pos()) delta = posic - audioBuffer.pos();
          else delta = (byteArray.size() + posic) - audioBuffer.pos();
      if (delta > (byteArray.size()/10)*9) {
          qDebug() << " delta > MAX";
          return;
      }
      if ((play_time_sec - audioOutput->processedUSecs()/1000000) > 1) {
          if (delta < int_buf_sze) {  //temporarily stop the audio until the buffers are full
              audioOutput->suspend();
              connect(reply_, &QIODevice::readyRead, this, &MainWindow::httpReadyRead);
          }
      }
      qDebug() << " delta  ->" << delta;
      QByteArray data;
      int32_t sze;
      sze = reply_->bytesAvailable();
      if ((posic + sze) > byteArray.size()) {
          sze = byteArray.size() - posic;
      }
      data = reply_->read(sze);
      byteArray.replace(posic, sze, data);
      posic+= sze;
      if (posic >= byteArray.size()) {
          posic = 0;
      }

  }

  void MainWindow::on_url_server_currentIndexChanged(const QString &arg1) {
      if (arg1 == "RESET LIST") {
          QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, ".");
          QSettings settings("recent_srv.ini", QSettings::IniFormat);
          QStringList recentServers =
                  settings.value("recentServers").toStringList();

          while (recentServers.size() > 0)
              recentServers.removeLast();
          if (!settings.isWritable() || settings.status() != QSettings::NoError)
              qDebug() << " error ->" << settings.status();
          settings.setValue("recentServers", recentServers);
          settings.sync();
          QString tmp = "";
          adjustForCurrentServers(tmp);
      }
  }

  int main(int argc, char *argv[]) {
      QApplication a(argc, argv);
          MainWindow w;
          w.setWindowIcon(QIcon(":/ico/mic_wifi.ico"));
          w.show();
#ifdef __ANDROID__
//          w.setFixedHeight(500);
//          w.setFixedWidth(900);
#endif



          return a.exec();
  }
