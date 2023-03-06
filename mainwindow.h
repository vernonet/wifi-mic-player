#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QtGui>
#include <QtNetwork>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QApplication>
#include <QDialog>
#include <QMainWindow>
#include <QAudioInput>
#include <QAudioOutput>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QProgressBar>
#include <QSlider>
#include <QLabel>


#define SIZE_AUDIO_BUF_IN_SEC    1/5     //1 sec
#define TCP_READ_BUFF_SIZE       (8192*2)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void play();
  void replyFinished(QNetworkReply *reply);
  void httpFinished();
  void httpReadyRead();
  void on_Volume_snd_valueChanged(int value);
  int processing_wav_header(char *header_ptr);
  void handleStateChanged(QAudio::State state);
  void handleNotify();
  void updateRecentServers();
  void adjustForCurrentServers(QString &server_str);
  void onAuthenticationRequest(QNetworkReply *,QAuthenticator *aAuthenticator);
  qreal sound_volume_setting(qreal snd_vol);
  void on_url_server_currentIndexChanged(const QString &arg1);

private:
  QAudioOutput *audioOutput = nullptr;
  QNetworkAccessManager *networkManager;
  QTcpSocket *socket;
  QBuffer audioBuffer;
  QAudioFormat format;
  QNetworkReply *reply_;
  bool started = false;
  QByteArray byteArray;
  QMediaPlayer *player;
  Ui::MainWindow *ui;
  uint32_t play_time_sec = 0;
  uint32_t play_time_coef;
  bool play_started = false;
  qint32 posic = 0;
  uint8_t maxSrvNr = 7;
  qint32 delta;
  qint32 int_buf_sze=0;
  char wav_hdr[44];
  QString url_str ="";

};




#endif // MAINWINDOW_H

 
