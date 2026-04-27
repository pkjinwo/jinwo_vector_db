package com.jinwo.vecdb.demo;

import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.TextView;
import android.widget.ScrollView;
import android.widget.LinearLayout;

public class MainActivity extends AppCompatActivity {
    private TextView outputText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        outputText = findViewById(R.id.output_text);

        runDemo();
    }

    private void runDemo() {
        StringBuilder output = new StringBuilder();
        output.append("========================================\n");
        output.append("  JinWo VecDB Android 演示程序\n");
        output.append("========================================\n\n");

        try {
            String dbPath = getFilesDir().getAbsolutePath() + "/vecdb";

            output.append("打开数据库...\n");
            long dbPtr = nativeOpenDatabase(dbPath, true);
            if (dbPtr == 0) {
                output.append("打开数据库失败\n");
                updateOutput(output.toString());
                return;
            }
            output.append("数据库打开成功\n");

            String version = nativeGetVersion();
            output.append("版本: " + version + "\n\n");

            output.append("创建集合...\n");
            long collectionPtr = nativeCreateCollection(dbPtr, "test", 128);
            if (collectionPtr == 0) {
                output.append("创建集合失败\n");
                nativeCloseDatabase(dbPtr);
                updateOutput(output.toString());
                return;
            }
            output.append("集合创建成功: test\n\n");

            output.append("插入10个向量...\n");
            for (int i = 0; i < 10; i++) {
                float[] vector = generateRandomVector(128);
                long id = nativeInsertVector(collectionPtr, vector);
                if (id == 0) {
                    output.append("插入向量 " + i + " 失败\n");
                } else {
                    output.append("插入向量 " + i + " 成功，ID: " + id + "\n");
                }
            }
            output.append("插入完成\n\n");

            output.append("搜索相似向量...\n");
            float[] query = generateRandomVector(128);
            long[] ids = nativeSearchVectorIds(collectionPtr, query, 5);
            float[] distances = nativeGetSearchDistances(collectionPtr, query, 5);
            if (ids != null && ids.length > 0) {
                output.append("搜索结果 (前" + ids.length + "个):\n");
                for (int i = 0; i < ids.length; i++) {
                    output.append("ID: " + ids[i] + ", 距离: " + String.format("%.4f", distances[i]) + "\n");
                }
            } else {
                output.append("搜索失败，无结果\n");
            }

            nativeCloseCollection(collectionPtr);
            nativeCloseDatabase(dbPtr);

            output.append("\n演示完成!");
        } catch (Exception e) {
            output.append("错误: " + e.getMessage() + "\n");
        }

        updateOutput(output.toString());
    }

    private float[] generateRandomVector(int dimension) {
        float[] vector = new float[dimension];
        for (int i = 0; i < dimension; i++) {
            vector[i] = (float) (Math.random() * 2.0 - 1.0);
        }
        return vector;
    }

    private void updateOutput(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                outputText.setText(text);
            }
        });
    }

    private native long nativeOpenDatabase(String path, boolean create);
    private native void nativeCloseDatabase(long dbPtr);
    private native String nativeGetVersion();
    private native long nativeCreateCollection(long dbPtr, String name, int dimension);
    private native void nativeCloseCollection(long collectionPtr);
    private native long nativeInsertVector(long collectionPtr, float[] vector);
    private native long[] nativeSearchVectorIds(long collectionPtr, float[] query, int k);
    private native float[] nativeGetSearchDistances(long collectionPtr, float[] query, int k);

    static {
        System.loadLibrary("jw_vecdb");
    }
}
