package com.embe;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import com.embe.LocalService.LocalBinder;
import com.embe.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.IBinder;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
	public static final String TAG = "ANDROID INSTRUMENTS"; //태그
	public static final int KEY_MARGIN = 1; // 건반간 간격 픽셀
	public static final int KEY_LENGTH = 7; // C Major 스케일 음계의 수
	public static final int OCTAVE_COUNT = 6; // 옥타브 수
	public static final String KEYS = "cdefgab"; // C Major 스케일 음계
	public static final String[] PITCHES = new String[]{"c", "cs", "d", "ds", "e", "f", "fs", "g", "gs", "a", "as", "b"}; // Chromatic 스케일 음계
	public native int add(int x, int y);
	public native void testString(String str);
	public native void metronome();
	public native void drum();
	
	// General MIDI Programs
	
	public static final String[] GM_PROGRAMS = new String[] {
		//"Electric Piano 2" 
	};
	 	
	private ViewGroup viewKeys, keysContainer, swipeArea;
	private ImageView imgKey;
	
	private SoundPool sPool; // 사운드 풀
	private Map<String, Integer> sMap;
	private AudioManager mAudioManager;
	
	private int imgKeyWidth, programNo, octaveShift;
	private SharedPreferences pref;
	
	@SuppressWarnings("unused")
	private int sKey0, sKey1, sKey2;
	
	LocalService mService;
    boolean mBound = false;
    
    public static Context mContext;
    
    int beatOn;
    MediaPlayer metronomeSound;
    SoundPool soundDrum;
    int soundDrumId;
    int drumPlayed = 0;
    
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		int x = 1000;
	    int y = 42;
	    
	    System.loadLibrary("ndk-prj");
	    
	    int z = add(x, y);
	    System.out.println("x + y = " + z);
	    
		pref = getSharedPreferences("net.hyeongkyu.android.androInstruments", Activity.MODE_PRIVATE);
		
		setContentView(R.layout.main);		

		viewKeys = (ViewGroup) findViewById(R.id.view_key);
		viewKeys.setOnTouchListener(keysTouchListener);
		
		keysContainer = (ViewGroup) findViewById(R.id.keys_container);
		keysContainer.setOnTouchListener(keysContainerTouchListener);
		int viewKeysScrollX = pref.getInt("viewKeysScrollX", 0);
		if(viewKeysScrollX>0)((HorizontalScrollView)viewKeys).smoothScrollTo(viewKeysScrollX, 0);
		
		imgKey = (ImageView) findViewById(R.id.img_key);
		imgKeyWidth = pref.getInt("imgKeyWidth", 4000);
		imgKey.setLayoutParams(new LinearLayout.LayoutParams(imgKeyWidth, LayoutParams.FILL_PARENT));
		
		//swipeArea = (ViewGroup) findViewById(R.id.swipe_area);
		
		resetSoundPool();
		mAudioManager = (AudioManager)getSystemService(AUDIO_SERVICE);
		
		boolean isFirstExec = pref.getBoolean("isFirstExec", true);
		if(isFirstExec){
			//showAbout();
			SharedPreferences.Editor editor = pref.edit();
			editor.putBoolean("isFirstExec", false);
			editor.commit();
		}
		
		mContext = this;
		
		Intent intent = new Intent(this, LocalService.class);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        
        metronomeSound = MediaPlayer.create(this,  R.raw.metronome);
        
		Button btn;
		btn = (Button)findViewById(R.id.button2);
		OnClickListener listener = new OnClickListener() {
			public void onClick(View v) {
				System.out.println("Button clicked!");
				if (mBound) {
					// Call a method from the LocalService.
					// However, if this call were something that might hang, then this request should
					// occur in a separate thread to avoid slowing down the activity performance.
					mService.supermario();
					//Toast.makeText(this, "number: " + num, Toast.LENGTH_SHORT).show();
				}
			}
		};
		btn.setOnClickListener(listener);
		
		Button btn2;
		btn2 = (Button)findViewById(R.id.button1);
		OnClickListener listener2 = new OnClickListener() {
			public void onClick(View v) {
				System.out.println("Button2 clicked!");
				ExampleThread thread = new ExampleThread();
				thread.start();
				ExampleThread2 thread2 = new ExampleThread2();
				thread2.start();
			}
		};
		btn2.setOnClickListener(listener2);
		
		// Create SoundPool
		soundDrum = new SoundPool(1, AudioManager.STREAM_MUSIC, 0);
		
		// load SoundPool
		soundDrumId = soundDrum.load(this, R.raw.drum, 1);
		
		// poll drum play
		/*drum();
		
		// polled data (drum) listener
		ThreadDrum threadDrum = new ThreadDrum();
		threadDrum.start();
		*/
	}
	
	private class ExampleThread extends Thread {
		public ExampleThread() {
			
		}
		public void run() {
			//System.out.println("Run!");
			metronome();
		}
	}
	
	private class ExampleThread2 extends Thread {
		
		public ExampleThread2() {
			beatOn = 0;
		}
		public void run() {
			while(beatOn != -1) {
				//System.out.println("Run!");
				if(beatOn == 1) {
					// ToDo: 
					System.out.println("Yes!");
					if(!metronomeSound.isPlaying()) 
						metronomeSound.start();
					try {
						sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					beatOn = 0;
				}
				else if(beatOn == 0) {
					System.out.println("No!");
					/*
					if(metronomeSound.isPlaying())
						metronomeSound.pause();
						*/
				}
				if(metronomeSound.isPlaying())
					metronomeSound.pause();
			}
		}
	}
	
private class ThreadDrum extends Thread {
		int streamId;
		public ThreadDrum() {
			
		}
		public void run() {
			while(drumPlayed != -1) {
				//System.out.println("Run!");
				if(drumPlayed == 1) {
					System.out.println("Drum!");
					soundDrum.play(soundDrumId, 1.0F, 1.0F,  1,  0,  1.0F);
				}
				/*lse if(drumPlayed == 0) {
					System.out.println("No!");
					if(metronomeSound.isPlaying())
						metronomeSound.pause();
				}*/
				//if(metronomeSound.isPlaying())
				//	metronomeSound.pause();
			}
		}
	}
	
	private OnTouchListener keysTouchListener = new OnTouchListener() {
		@Override
		public boolean onTouch(View view, MotionEvent event) {
			int action = event.getAction();
			int downPointerIndex = -1;
			int upPointerIndex = -1;
			
			if (action == MotionEvent.ACTION_DOWN) downPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_DOWN) downPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_1_DOWN) downPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_2_DOWN) downPointerIndex = 1;
			else if(action == MotionEvent.ACTION_POINTER_3_DOWN) downPointerIndex = 2;
			else if(action == MotionEvent.ACTION_UP) upPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_UP) upPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_1_UP) upPointerIndex = 0;
			else if(action == MotionEvent.ACTION_POINTER_2_UP) upPointerIndex = 1;
			else if(action == MotionEvent.ACTION_POINTER_3_UP) upPointerIndex = 2;
			
			//int scrollAreaBottom = swipeArea.getBottom(); // 스크롤 영역의 하단
			int scrollAreaBottom = 60; // 스크롤 영역의 하단
			//System.out.println("scrollAreaBottom: "+scrollAreaBottom);
			
			if(downPointerIndex>=0){
				int scrollWidth = keysContainer.getRight(); // 스크롤 폭
				int keyWhiteWidth = (int) (((float) scrollWidth) / (KEY_LENGTH * OCTAVE_COUNT)); // 건반 하나당 할당 폭
				int octaveWidth = (int)((float)scrollWidth/OCTAVE_COUNT); // 옥타브당 폭
				
				//System.out.println("scrollWidth: "+scrollWidth);
				
				//int scrollX = view.getScrollX(); // 스크롤 위치
				int scrollX = 0; // 스크롤 위치
				int bottom = view.getBottom()-scrollAreaBottom; // 건반 높이
				//int bottom = view.getBottom()-scrollAreaBottom; // 건반 높이
				
				System.out.println("scrollX: "+scrollX);
				
				float touchX = event.getX(downPointerIndex);
				float touchY = event.getY(downPointerIndex);
				//float touchY = event.getY(downPointerIndex)-scrollAreaBottom;
				if(touchY<0) return false;
				
				System.out.println("touchX: "+touchX);
				
				int touchKeyX = 1335 + (int) touchX; // 이미지 상의 터치 X 좌표
				int touchKeyPos = (touchKeyX / keyWhiteWidth); // 몇번째 흰 건반인가
				int touchYPosPercent = (int)((touchY/((float)bottom))*100); // Y좌표는 height 대비  몇 % 지점에 찍혔는가
				
				System.out.println("touchKeyX: "+touchKeyX);
				
				//옥타브 계산
				int octave = (touchKeyX/octaveWidth)+1;
				
				String key = ""+KEYS.charAt(touchKeyPos % (KEY_LENGTH));
				if(touchYPosPercent<55){
					//전체 높이의 55% 이내에 찍혔으면, 검은 건반을 눌렀을 가능성이 있음.
					//각 흰 건반의 경계로부터 30% 이내에 맞았을 경우, 건반의 경계를 파악하여, 그 자리에 검은 건반이 있는지 확인한다.
					int nearLineX1 = ((touchKeyX/keyWhiteWidth)*keyWhiteWidth);
					int nearLineX2 = (((touchKeyX/keyWhiteWidth)+1)*keyWhiteWidth);
					if((touchKeyX-nearLineX1)<(nearLineX2-touchKeyX)){
						//아랫쪽 건반에 가까울 경우
						if(((touchKeyX-nearLineX1)/(float)keyWhiteWidth)<0.3f){
							//검은 건반 유효(flat)
							if("cf".indexOf(key)<0){
								int keyCharPos = KEYS.indexOf(key)-1;
								if(keyCharPos<0) keyCharPos = KEY_LENGTH;
								key = ""+KEYS.charAt(keyCharPos);
								key += "s";
							}
						}
					}else{
						//윗쪽 건반에 가까울 경우
						if(((nearLineX2-touchKeyX)/(float)keyWhiteWidth)<0.3f){
							//검은 건반 유효(sharp)
							if("eb".indexOf(key)<0) key += "s";
						}
					}
				}
				key += octave;
				testString(key);	// use device driver via NDK
				
				
				//////////////////////////////////////////////////////////////////////
				int soundKey = sMap.get(key);
				int streamVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
				int sKey = sPool.play(soundKey, streamVolume, streamVolume, 0, 0, 1);
				
				if(downPointerIndex==0) sKey0 = sKey;
				else if(downPointerIndex==1) sKey1 = sKey;
				else if(downPointerIndex==2) sKey2 = sKey;
			}
			
			//터치 영역이 스크롤 영역인지 확인
			float touchY = event.getY();
			if(touchY>scrollAreaBottom) return true;
			return false;
		}
	};
	
	private OnTouchListener keysContainerTouchListener = new OnTouchListener() {
		@Override
		public boolean onTouch(View v, MotionEvent event) {
			return false;
		}
	};
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		switch (keyCode) {
		case KeyEvent.KEYCODE_VOLUME_DOWN:
			mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
			return true;
		case KeyEvent.KEYCODE_VOLUME_UP:
			mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}
	
	/**
	 * Sound Pool을 재설정합니다.
	 */
	
	private void resetSoundPool(){
		final ProgressDialog progress = new ProgressDialog(this);
		progress.setMessage("악기를 설정 중입니다.");
		
		Thread thread = new Thread(new Runnable() {
			@Override
			public void run() {
				Map<String, Integer> tmpMap = new HashMap<String, Integer>();
				SoundPool tmpPool = new SoundPool(3, AudioManager.STREAM_MUSIC, 0); 
								
				//미디 파일 생성
				try {
					programNo = pref.getInt("programNo", 1);
					octaveShift = pref.getInt("octaveShift", 0);
					MidiFileCreator midiFileCreator = new MidiFileCreator(MainActivity.this);
					midiFileCreator.createMidiFiles(programNo, octaveShift);
				} catch (IOException e) {
					e.printStackTrace();
				}
				
				String dir = getDir("", MODE_PRIVATE).getAbsolutePath();
				for(int i=1;i<=OCTAVE_COUNT;i++){
					for (int j=0;j<PITCHES.length;j++){
						String soundPath = dir+File.separator+PITCHES[j]+i+".mid";
						tmpMap.put(PITCHES[j]+i, tmpPool.load(soundPath, 1));
					}
				}
				
				sMap = tmpMap;
				sPool = tmpPool;
				
				progress.dismiss();
			}
		});
		progress.show();
		thread.start();
	}
	
	 /** Defines callbacks for service binding, passed to bindService() */
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            // We've bound to LocalService, cast the IBinder and get LocalService instance
            LocalBinder binder = (LocalBinder) service;
            mService = binder.getService();
            mBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            mBound = false;
        }
    };
	
	@Override
	protected void onStop() {
		// 현재 스크롤 상태를 저장합니다.
		SharedPreferences.Editor editor = pref.edit();
		editor.putInt("viewKeysScrollX", viewKeys.getScrollX());
		editor.commit();
		super.onStop();
		if (mBound) {
            unbindService(mConnection);
            mBound = false;
        }
	}
}