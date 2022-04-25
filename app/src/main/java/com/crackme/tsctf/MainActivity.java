package com.crackme.tsctf;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.widget.Toast;

import com.andrognito.patternlockview.PatternLockView;
import com.andrognito.patternlockview.listener.PatternLockViewListener;
import com.andrognito.patternlockview.utils.PatternLockUtils;

import java.util.List;

public class MainActivity extends AppCompatActivity {
    public Context context = this;
    public PatternLockView mPatternLockView;

    private PatternLockViewListener mPatternLockViewListener = new PatternLockViewListener() {
        @Override
        public void onStarted() {}

        @Override
        public void onProgress(List<PatternLockView.Dot> progressPattern) {
        }

        @Override
        public void onComplete(List<PatternLockView.Dot> pattern) {
            String str = PatternLockUtils.patternToSha1(mPatternLockView, pattern);

            if(TsUtil.check(str)) {
                mPatternLockView.setViewMode(PatternLockView.PatternViewMode.CORRECT);
                Toast.makeText(context,"Submit TSCTF{" + str + "}",Toast.LENGTH_SHORT).show();
            }
            else mPatternLockView.setViewMode(PatternLockView.PatternViewMode.WRONG);
            
        }

        @Override
        public void onCleared() {
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mPatternLockView = (PatternLockView) findViewById(R.id.pattern_lock_view);
        TsUtil.loadclass(this.getApplicationContext());
        mPatternLockView.addPatternLockListener(mPatternLockViewListener);

    }

}