package net.kulthx.nizarcrash3recomp;

import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import org.libsdl.app.SDLActivity;

/**
 * Android entry point for the PSXRecomp runtime build.
 * SDL is linked into libmain.so for this native Android target.
 */
public final class NizarPsxRuntimeActivity extends SDLActivity {
    private static final String TAG = "NizarCrash3";
    private static final String BIOS_ASSET = "bios/openbios.bin";

    /**
     * PSXRecomp expects bios/openbios.bin as a normal file next to its runtime
     * data. Android APK assets are read-only streams, so materialize the
     * bundled image into this app's private files directory before SDL starts.
     */
    private void installBundledBios() {
        File biosDir = new File(getFilesDir(), "bios");
        File target = new File(biosDir, "openbios.bin");
        File temporary = new File(biosDir, "openbios.bin.tmp");

        try {
            if (!biosDir.isDirectory() && !biosDir.mkdirs() && !biosDir.isDirectory()) {
                throw new IOException("Could not create private BIOS directory: " + biosDir);
            }

            long total = 0;
            byte[] buffer = new byte[32 * 1024];
            try (InputStream input = getAssets().open(BIOS_ASSET);
                 FileOutputStream output = new FileOutputStream(temporary, false)) {
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                    total += count;
                }
                output.getFD().sync();
            }

            if (total <= 0) {
                throw new IOException("Bundled OpenBIOS asset is empty");
            }

            if (target.exists() && !target.delete()) {
                throw new IOException("Could not replace old BIOS file: " + target);
            }
            if (!temporary.renameTo(target)) {
                throw new IOException("Could not move BIOS into place: " + target);
            }
            Log.i(TAG, "Bundled OpenBIOS installed (" + total + " bytes): " + target);
        } catch (IOException error) {
            if (temporary.exists() && !temporary.delete()) {
                Log.w(TAG, "Could not remove incomplete BIOS temporary file");
            }
            Log.e(TAG, "Could not install bundled OpenBIOS before SDL startup", error);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        installBundledBios();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }
}
