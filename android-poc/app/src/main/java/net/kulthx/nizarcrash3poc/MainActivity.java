package net.kulthx.nizarcrash3poc;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private static final String TAG = "FIBER_POC";

    static {
        System.loadLibrary("psx_fiber_poc");
    }

    private native int nativeRunFiberSmokeTest();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        TextView status = new TextView(this);
        status.setTextSize(18f);
        status.setPadding(32, 48, 32, 32);
        status.setText("Running PSXRecomp Android fiber test...");
        setContentView(status);

        final int result;
        try {
            result = nativeRunFiberSmokeTest();
        } catch (Throwable error) {
            Log.e(TAG, "FAIL: native fiber test threw an exception", error);
            status.setText("FAIL: native fiber test threw an exception\n" + error);
            return;
        }

        if (result == 0) {
            Log.i(TAG, "PASS: Android ucontext fiber switching and stack preservation");
            status.setText("PASS\n\nPSXRecomp's pinned ucontext fiber backend ran on Android.");
        } else {
            Log.e(TAG, "FAIL: native fiber test returned " + result);
            status.setText("FAIL\n\nNative test result: " + result);
        }
    }
}
