package com.embe;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.Binder;
import android.os.IBinder;

public class LocalService extends Service {
	// Binder given to clients
    private final IBinder mBinder = new LocalBinder();
    
    
    /**
     * Class used for the client Binder.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with IPC.
     */
    public class LocalBinder extends Binder {
        LocalService getService() {
            // Return this instance of LocalService so clients can call public methods
            return LocalService.this;
        }
    }

    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /*public Context mContext = null;
    
    public LocalService(Context context) {
    	super();
    	this.mContext = context;
    }*/
    
    /** method for clients */
    public void supermario() {
    	System.out.println("super mario!");
    	Intent intent = new Intent(LocalService.this, IntentShow.class);
    	intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    	startActivity(intent);
    }
}
