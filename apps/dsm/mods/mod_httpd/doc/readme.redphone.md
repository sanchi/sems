# RedPhone - SIP gateway

work in progress!!! this doesn't work yet.

see https://github.com/WhisperSystems/RedPhone/wiki/Signaling-Protocol

what works
 * receive Initiate signal, respond properly
 * create RTP relay
 * respond with 200 to http/udp hole punch
 * send request with MHD (probably)
 
current issues
 * libredspeex (jni) crashes in avd (see below)
 * extend libmicrohttpd to process replies for requests sent as client
 * how to get to mhd server thread to send request? (use suspend?)

todo:
 * complete call signals (ringing, busy, hangup)
 * make redphone use RTP timestamp
 * make redphone negotiate payload IDs (needs slight protocol change)
 * implement auth header in http server
 * make RTP relay optional (whispersystems' gateways already do relay)
 * inbound calls
 
install and configuration (assuming debian/ubuntu)
--------------------------------------------------

  apt-get install libevent-dev g++ make
  git clone https://github.com/sanchi/sems.git
  cd sems
  pushd apps/dsm/mods/mod_httpd
  wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.33.tar.gz ; tar xzvf libmicrohttpd-0.9.33.tar.gz ; patch -p0 < mhd_patch_clientreq.patch
  popd
  make -C core
  make -C apps/dsm
  cd core
  openssl genrsa -out key.pem 1024
  openssl req -days 365 -out cert.pem -new -x509 -key key.pem

  cp etc/sems.conf.sample sems.conf
  cp ../apps/dsm/etc/dsm.conf etc/
  cp ../apps/dsm/mods/mod_httpd/etc/mod_httpd.conf etc/

  ./sems -f etc/sems.conf -D 3 -E

sems.conf changes
-----------------
 load_plugins=dsm
 plugin_config_path=etc/
 plugin_path=lib/

dsm.conf:
--------
 conf_dir=../apps/dsm/mods/mod_httpd/redphone
 redphone_apps=redphone_call

Redphone (in emulator AVD)
--------------------------
- open whisper.store with portecle (http://portecle.sourceforge.net/), add cert.pem, save store
- patch redphone: 
<pre>
src/org/thoughtcrime/redphone/signaling/signals/Signal.java:
diff --git a/src/org/thoughtcrime/redphone/signaling/signals/Signal.java b/src/org/thoughtcrime/redphone/signaling/signals/Signal.java
index 79321c8..4a39996 100644
--- a/src/org/thoughtcrime/redphone/signaling/signals/Signal.java
+++ b/src/org/thoughtcrime/redphone/signaling/signals/Signal.java
@@ -81,7 +81,7 @@ public abstract class Signal {
     sb.append(getMethod());
     sb.append(" ");
     sb.append(getLocation());
-    sb.append(" HTTP/1.0\r\n");
+    sb.append(" HTTP/1.1\r\n");
   }
 
   protected abstract String getMethod();

diff --git a/src/org/thoughtcrime/redphone/Release.java b/src/org/thoughtcrime/redphone/Release.java
index 15eec64..592df65 100644
--- a/src/org/thoughtcrime/redphone/Release.java
+++ b/src/org/thoughtcrime/redphone/Release.java
@@ -24,12 +24,12 @@ package org.thoughtcrime.redphone;
  *
  */
 public interface Release {
-  public static final boolean SSL                     = true;
-  public static final boolean DEBUG                   = false;
+  public static final boolean SSL                     = false; //true;
+  public static final boolean DEBUG                   = true; //false;
   public static final boolean DELIVER_DIAGNOSTIC_DATA = false;
-  public static final String  SERVER_ROOT             = ".whispersystems.org";
+  public static final String  SERVER_ROOT             = ""; //".whispersystems.org";
   public static final String MASTER_SERVER_HOST       = "master.whispersystems.org";
-  public static final String RELAY_SERVER_HOST        = "relay.whispersystems.org";
+  public static final String RELAY_SERVER_HOST        = "10.0.2.2"; //"relay.whispersystems.org";
   public static final String DATA_COLLECTION_SERVER_HOST = "redphone-call-metrics.herokuapp.com";
   public static final int     SERVER_PORT             = 31337;
 }

</pre>
---------------------------------------------------------------------------------------------
AVD JNI exception:

<pre>
D/ATM     (29766): track initialized, buffer size = 16742
D/dalvikvm(29766): Trying to load lib /data/app-lib/org.thoughtcrime.redphone-2/libredspeex.so 0xb3d5e808
W/linker  (29766): libredspeex.so has text relocations. This is wasting memory and is a security risk. Please fix.
D/dalvikvm(29766): Added shared lib /data/app-lib/org.thoughtcrime.redphone-2/libredspeex.so 0xb3d5e808
D/dalvikvm(29766): No JNI_OnLoad found in /data/app-lib/org.thoughtcrime.redphone-2/libredspeex.so 0xb3d5e808, skipping init
D/SpeexCodec(29766): loaded redspeex, now opening it
W/dalvikvm(29766): JNI WARNING: illegal class name 'android.util.Log' (FindClass)
W/dalvikvm(29766):              (should be formed like 'dalvik/system/DexFile')
W/dalvikvm(29766):              or '[Ldalvik/system/DexFile;' or '[[B')
I/dalvikvm(29766): "Thread-110" prio=5 tid=18 NATIVE
I/dalvikvm(29766):   | group="main" sCount=0 dsCount=0 obj=0xb4254500 self=0xb8d0d090
I/dalvikvm(29766):   | sysTid=29888 nice=0 sched=0/0 cgrp=apps handle=-1194273560
I/dalvikvm(29766):   | state=R schedstat=( 10000000 250000000 13 ) utm=0 stm=1 core=0
W/CallManager(29766): org.thoughtcrime.redphone.signaling.SignalingException: java.io.IOException: Socket closed before buffer filled...
W/CallManager(29766):   at org.thoughtcrime.redphone.signaling.SignalingSocket.readSignal(SignalingSocket.java:366)
W/CallManager(29766):   at org.thoughtcrime.redphone.call.SignalManager$SignalListenerTask.run(SignalManager.java:77)
W/CallManager(29766):   at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1112)
W/CallManager(29766):   at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:587)
W/CallManager(29766):   at java.lang.Thread.run(Thread.java:841)
W/CallManager(29766): Caused by: java.io.IOException: Socket closed before buffer filled...
W/CallManager(29766):   at org.thoughtcrime.redphone.util.LineReader.readFully(LineReader.java:122)
W/CallManager(29766):   at org.thoughtcrime.redphone.signaling.SignalReader.readSignalBody(SignalReader.java:80)
W/CallManager(29766):   at org.thoughtcrime.redphone.signaling.SignalingSocket.readSignal(SignalingSocket.java:362)
W/CallManager(29766):   ... 4 more
W/RedPhone(29766): Got message from service: 6
W/RedPhone(29766): handleTerminate called
W/RedPhone(29766): Termination Stack:
W/RedPhone(29766): java.lang.Exception
W/RedPhone(29766):      at org.thoughtcrime.redphone.RedPhone.handleTerminate(RedPhone.java:242)
W/RedPhone(29766):      at org.thoughtcrime.redphone.RedPhone.access$600(RedPhone.java:70)
W/RedPhone(29766):      at org.thoughtcrime.redphone.RedPhone$CallStateHandler.handleMessage(RedPhone.java:414)
W/RedPhone(29766):      at android.os.Handler.dispatchMessage(Handler.java:102)
W/RedPhone(29766):      at android.os.Looper.loop(Looper.java:136)
W/RedPhone(29766):      at android.app.ActivityThread.main(ActivityThread.java:5017)
W/RedPhone(29766):      at java.lang.reflect.Method.invokeNative(Native Method)
W/RedPhone(29766):      at java.lang.reflect.Method.invoke(Method.java:515)
W/RedPhone(29766):      at com.android.internal.os.ZygoteInit$MethodAndArgsCaller.run(ZygoteInit.java:779)
W/RedPhone(29766):      at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:595)
W/RedPhone(29766):      at dalvik.system.NativeStart.main(Native Method)
W/RedPhoneService(29766): termination stack
W/RedPhoneService(29766): java.lang.Exception
W/RedPhoneService(29766):       at org.thoughtcrime.redphone.RedPhoneService.terminate(RedPhoneService.java:367)
W/RedPhoneService(29766):       at org.thoughtcrime.redphone.RedPhoneService.notifyCallDisconnected(RedPhoneService.java:502)
W/RedPhoneService(29766):       at org.thoughtcrime.redphone.call.SignalManager$SignalListenerTask.run(SignalManager.java:92)
W/RedPhoneService(29766):       at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1112)
W/RedPhoneService(29766):       at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:587)
W/RedPhoneService(29766):       at java.lang.Thread.run(Thread.java:841)
D/LockManager(29766): Entered Lock State: PARTIAL
D/AccelerometerListener(29766): enable(false)
D/CallDataImpl(29766): Adding Event: {"values":{"call-setup":"start-negotiate"},"timestamp":62818704}
W/ZRTPSocket(29766): Retransmitting after: 150
W/ZRTPSocket(29766): Sending Packet: null
D/org.thoughtcrime.redphone.audio.IncomingRinger(29766): Cancelling vibrator
W/ZRTPSocket(29766): Retransmitting after: 300
W/ZRTPSocket(29766): Sending Packet: null
I/dalvikvm(29766):   #00  pc 0000132e  /system/lib/libcorkscrew.so (unwind_backtrace_thread+29)
I/dalvikvm(29766):   #01  pc 000603ea  /system/lib/libdvm.so (dvmDumpNativeStack(DebugOutputTarget const*, int)+33)
I/dalvikvm(29766):   #02  pc 000543e4  /system/lib/libdvm.so (dvmDumpThreadEx(DebugOutputTarget const*, Thread*, bool)+395)
I/dalvikvm(29766):   #03  pc 00054452  /system/lib/libdvm.so (dvmDumpThread(Thread*, bool)+25)
I/dalvikvm(29766):   #04  pc 0003871c  /system/lib/libdvm.so
I/dalvikvm(29766):   #05  pc 000416c8  /system/lib/libdvm.so
I/dalvikvm(29766):   #06  pc 00010666  /data/app-lib/org.thoughtcrime.redphone-2/libredspeex.so (logv(_JNIEnv*, char const*, ...)+49)
I/dalvikvm(29766):   #07  pc 000109be  /data/app-lib/org.thoughtcrime.redphone-2/libredspeex.so (Java_org_thoughtcrime_redphone_codec_SpeexCodec_openSpeex+29)
D/CallDataImpl(29766): Adding Event: {"values":{"call-setup":"terminate"},"timestamp":62819286}
I/dalvikvm(29766):   #08  pc 0001dbcc  /system/lib/libdvm.so (dvmPlatformInvoke+112)
I/dalvikvm(29766):   #09  pc 0004defe  /system/lib/libdvm.so (dvmCallJNIMethod(unsigned int const*, JValue*, Method const*, Thread*)+393)
I/dalvikvm(29766):   #10  pc 0003873c  /system/lib/libdvm.so (dvmCheckCallJNIMethod(unsigned int const*, JValue*, Method const*, Thread*)+7)
I/dalvikvm(29766):   #11  pc 0004f8ea  /system/lib/libdvm.so (dvmResolveNativeMethod(unsigned int const*, JValue*, Method const*, Thread*)+181)
I/dalvikvm(29766):   #12  pc 00026fe0  /system/lib/libdvm.so
I/dalvikvm(29766):   #13  pc 0002df34  /system/lib/libdvm.so (dvmMterpStd(Thread*)+76)
I/dalvikvm(29766):   #14  pc 0002b5cc  /system/lib/libdvm.so (dvmInterpret(Thread*, Method const*, JValue*)+184)
I/dalvikvm(29766):   #15  pc 00060318  /system/lib/libdvm.so (dvmCallMethodV(Thread*, Method const*, Object*, bool, JValue*, std::__va_list)+335)
I/dalvikvm(29766):   #16  pc 0006033c  /system/lib/libdvm.so (dvmCallMethod(Thread*, Method const*, Object*, JValue*, ...)+19)
I/dalvikvm(29766):   #17  pc 0005502a  /system/lib/libdvm.so
I/dalvikvm(29766):   #18  pc 0000d060  /system/lib/libc.so (__thread_entry+72)
I/dalvikvm(29766):   #19  pc 0000d1f8  /system/lib/libc.so (pthread_create+240)
I/dalvikvm(29766):   at org.thoughtcrime.redphone.codec.SpeexCodec.openSpeex(Native Method)
I/dalvikvm(29766):   at org.thoughtcrime.redphone.codec.SpeexCodec$1.run(SpeexCodec.java:39)
I/dalvikvm(29766): 
E/dalvikvm(29766): VM aborting
F/libc    (29766): Fatal signal 6 (SIGABRT) at 0x00007446 (code=-6), thread 29888 (Thread-110)
I/DEBUG   (   49): *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
I/DEBUG   (   49): Build fingerprint: 'generic/sdk/generic:4.4.2/KK/938007:eng/test-keys'
I/DEBUG   (   49): Revision: '0'
I/DEBUG   (   49): pid: 29766, tid: 29888, name: Thread-110  >>> org.thoughtcrime.redphone <<<
I/DEBUG   (   49): signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
W/ProcessCpuTracker(  375): Skipping unknown process pid 29894

</pre>