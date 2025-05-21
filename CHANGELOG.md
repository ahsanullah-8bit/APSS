> [!NOTE]
> This file contains both previous and upcoming changelogs

## Future Versions

### APSS 0.2
APSS 0.2 as I'm thinking should include camera support (I don't own one, but lets see).

---

## Changelog

### APSS 0.1
APSS 0.1 is just a "gaining source control" release. Because everything is out of control and I'm doing
everything at same time and the dependencies won't come off togather.

So far, we have:
* Video footage display (not camera)
* Object Detection using YOLO11n
* License Plate Detection using a custom trained YOLO11n-pose model
* Some light-weight tracking using ByteTrackEigen, of objects, to avoid re-inference in further stages.
* OpenVINO EP, for faster inference
* APSS's Custom Controls Style setup
* Some more things, that shouldn't be part of this release but (I'm learning) and ... here they are.
    * QxOrm setup, needed for database.
	* QxOrm models
	* Maybe more ...
