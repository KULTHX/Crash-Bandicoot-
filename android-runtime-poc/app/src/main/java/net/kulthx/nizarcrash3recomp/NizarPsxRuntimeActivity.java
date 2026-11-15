package net.kulthx.nizarcrash3recomp;

import org.libsdl.app.SDLActivity;

/**
 * Android entry point for the PSXRecomp runtime build.
 * SDL is linked into libmain.so for this native Android target.
 */
public final class NizarPsxRuntimeActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }
}
