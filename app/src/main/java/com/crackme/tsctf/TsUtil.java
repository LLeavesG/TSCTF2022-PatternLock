package com.crackme.tsctf;

import android.content.Context;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;

import dalvik.system.DexClassLoader;

public class TsUtil {
    static {
        System.loadLibrary("native");
    }
    public static Class clz;
    public static native boolean check(String str);

    public static boolean cmp(String str){
        try {
            return (boolean) clz.getDeclaredMethod("cmp",String.class).invoke(0,str);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static void loadclass(Context context) {
        InputStream inputStream = null;
        try {
            inputStream = context.getAssets().open("classes.dex");
            byte[] buffer =new byte[inputStream.available()];
            inputStream.read(buffer);
            inputStream.close();
            String path = context.getApplicationInfo().dataDir + "/classes.dex";
            OutputStream outputStream = new BufferedOutputStream(new FileOutputStream(path));
            outputStream.write(buffer,0,buffer.length);
            outputStream.close();
            String dexPath = context.getDir("dex",0).getAbsolutePath();
            DexClassLoader loader = new DexClassLoader(path,dexPath,context.getApplicationInfo().nativeLibraryDir,context.getClassLoader());
            File dexfile = new File(path);
            if(dexfile.exists() && dexfile.isFile() ) {
                dexfile.delete();
            };
            clz = loader.loadClass("com.crackme.tsctf.Check");
        } catch (IOException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        System.loadLibrary("native");
    }

}
