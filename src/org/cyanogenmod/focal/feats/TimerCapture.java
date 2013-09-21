/*
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal.feats;

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.util.Log;

import org.cyanogenmod.focal.CameraActivity;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.ui.ShutterButton;

import java.util.ArrayList;

/**
 * Handles the timer before a capture, or the voice shutter feature
 */
public class TimerCapture extends CaptureTransformer implements RecognitionListener {
    public final static String TAG = "TimerCapture";
    public final static int VOICE_TIMER_VALUE = -1;

    private int mTimer;
    private CameraActivity mActivity;
    private SpeechRecognizer mSpeechRecognizer;
    private Intent mSpeechRecognizerIntent;
    private AudioManager mAudioManager;
    private String[] mShutterWords;
    private boolean mIsMuted;
    private boolean mIsInitialised;
    private CountDownTimer mCountDown;

    public TimerCapture(CameraActivity activity) {
        super(activity.getCamManager(), activity.getSnapManager());
        mActivity = activity;
        mIsInitialised = false;
        mIsMuted = false;
        mTimer = 5;
    }

    /**
     * Sets the amount of seconds before taking a shot, or VOICE_TIMER_VALUE for voice
     * trigger commands
     *
     * @param seconds Number of seconds before taking a shots, or VOICE_TIMER_VALUE
     */
    public void setTimer(int seconds) {
        if (seconds == mTimer) return;

        if (mTimer == VOICE_TIMER_VALUE) {
            // We were in voice shutter mode, disable it
            clearVoiceShutter();
        }

        mTimer = seconds;

        if (seconds == VOICE_TIMER_VALUE) {
            initializeVoiceShutter();
        }
    }

    public int getTimer() {
        return mTimer;
    }

    public void clearVoiceShutter() {
        //Log.d(TAG,"Stopping speach recog - " + mSpeechActive + "/" + enable);
        //mSpeechActive = false;
        if (mIsMuted) {
            mAudioManager.setStreamMute(AudioManager.STREAM_SYSTEM, false);
            mIsMuted = false;
        }

        //mPhotoModule.updateVoiceShutterIndicator(false);
        mSpeechRecognizer.cancel();
    }

    public void initializeVoiceShutter() {
        if (!mIsInitialised) {
            Context context =  mCamManager.getContext();

            if (context == null) {
                Log.e(TAG, "Could not initialise voice shutter because of null context");
            }

            mSpeechRecognizer = SpeechRecognizer.createSpeechRecognizer(context);
            mSpeechRecognizer.setRecognitionListener(this);

            mSpeechRecognizerIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
            mSpeechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                    RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
            mSpeechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_CALLING_PACKAGE,
                    "org.cyanogenmod.voiceshutter");
            mSpeechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 10);
            mSpeechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);


            mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

            mShutterWords = context.getResources().getStringArray(
                    R.array.transformer_timer_voice_words);
        }

        // Turn it on
        if (!mIsMuted) {
            /* Avoid beeps when re-arming the listener */
            mIsMuted = true;
            mAudioManager.setStreamMute(AudioManager.STREAM_SYSTEM, true);
        }

        //Log.d(TAG,"Starting speach recog");
        // TODO: mPhotoModule.updateVoiceShutterIndicator(true);
        mSpeechRecognizer.startListening(mSpeechRecognizerIntent);
    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {
        if (mTimer > 0) {
            mActivity.startTimerCountdown(mTimer * 1000);
            mCountDown = new CountDownTimer(mTimer * 1000, 1000) {
                @Override
                public void onTick(long l) {
                    updateTimerIndicator();
                }

                @Override
                public void onFinish() {
                    mSnapManager.queueSnapshot(true, 0);
                    mActivity.hideTimerCountdown();
                }
            };

            mCountDown.start();
        } else {
            mSnapManager.queueSnapshot(true, 0);
        }
    }

    @Override
    public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessing(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onMediaSavingStart() {

    }

    @Override
    public void onMediaSavingDone() {

    }

    @Override
    public void onVideoRecordingStart() {

    }

    @Override
    public void onVideoRecordingStop() {

    }

    @Override
    public void onReadyForSpeech(Bundle bundle) {
        Log.d(TAG, "ON READY FOR SPEECH");
        //mIsCountDownOn = true;
        //mNoSpeechCountDown.start();
        if (mIsMuted) {
            mAudioManager.setStreamMute(AudioManager.STREAM_SYSTEM, false);
            mIsMuted = false;
        }
    }

    @Override
    public void onBeginningOfSpeech() {

    }

    @Override
    public void onRmsChanged(float v) {

    }

    @Override
    public void onBufferReceived(byte[] bytes) {

    }

    @Override
    public void onEndOfSpeech() {

    }

    @Override
    public void onError(int error) {
        Log.e(TAG, "error " +  error);
        mIsInitialised = false;
        initializeVoiceShutter();
    }

    @Override
    public void onResults(Bundle bundle) {
        onPartialResults(bundle);
        initializeVoiceShutter();
    }

    @Override
    public void onPartialResults(Bundle partialResults) {
        ArrayList data = partialResults.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        if (data == null) {
            Log.e(TAG, "Null Partial Results");
            return;
        }
        String str = "";
        for (int i = 0; i < data.size(); i++) {
            Log.d(TAG, "result " + data.get(i));
            for (int f = 0; f < mShutterWords.length; f++) {
                String[] resultWords = data.get(i).toString().split(" ");
                for (int g = 0; g < resultWords.length; g++) {
                    if (mShutterWords[f].equalsIgnoreCase(resultWords[g])) {
                        Log.d(TAG, "matched to hotword! FIRE SHUTTER!");
                        mSnapManager.queueSnapshot(true, 0);
                        //mSpeechActive = false;
                        //enableSpeechRecognition(false, null);
                    }
                }
            }
            str += data.get(i);
        }
    }

    @Override
    public void onEvent(int i, Bundle bundle) {

    }

    public void updateTimerIndicator() {

    }
}
