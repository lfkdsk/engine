package io.flutter.view;
import java.io.*;
import java.util.Collection;
import java.util.HashSet;

public interface OnUpdateAotCallBack {
    boolean updateAotResource(HashSet<String> mResources);

    boolean shouldJumpDeleteResource();
}