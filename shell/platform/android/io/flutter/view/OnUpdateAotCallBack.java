package io.flutter.view;

public interface OnUpdateAotCallBack {
    boolean updateAotResource(HashSet<String> mResources);

    boolean shouldJumpDeleteResource();
}