The sample.g722 file was created by running...

```
gst-launch-1.0 audiotestsrc samplesperbuffer=320 num-buffers=1 ! avenc_g722 ! filesink location=out.g722
```
