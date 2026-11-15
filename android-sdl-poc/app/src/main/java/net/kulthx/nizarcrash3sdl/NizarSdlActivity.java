package net.kulthx.nizarcrash3sdl;

import org.libsdl.app.SDLActivity;

/** Thin Android entry point. SDL owns the native application lifecycle. */
public final class NizarSdlActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
}
