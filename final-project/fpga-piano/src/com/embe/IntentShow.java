package com.embe;

import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Bundle;
import android.widget.ImageView;

public class IntentShow extends Activity {
	MediaPlayer mario;
	
	/** Called when the activity is first created. */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main2);
		Drawable drawable = getResources().getDrawable(R.drawable.super1);
		ImageView imageView = (ImageView)findViewById(R.id.imageView1);
		Drawable drawable2 = getResources().getDrawable(R.drawable.super2);
		ImageView imageView2 = (ImageView)findViewById(R.id.imageView2);

		mario = MediaPlayer.create(this,  R.raw.supermario);
		mario.start();
		imageView.setImageDrawable(drawable);
		imageView2.setImageDrawable(drawable2);
		/*
		while(mario.isPlaying()) {
			
		}
		*/
		mario.setOnCompletionListener(new OnCompletionListener(){

			@Override
			public void onCompletion(MediaPlayer mp) {
				// TODO Auto-generated method stub
				Intent intent = new Intent(IntentShow.this, MainActivity.class);
		    	startActivity(intent);
			}
		});
		//Intent intent = new Intent(IntentShow.this, MainActivity.class);
    	//startActivity(intent);
	}
}
