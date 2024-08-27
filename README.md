# Multiprojector Projection Mapping for Indoor-Climbing

Projection system that allows for auto-calibration of arbitrary number of projectors with central camera.
**Work-in-progress**, calibration is currently not very robust for diverse projector positions and system is not solved for equally distributed brightness.
Contains some unfinished attempts to tackle issues specific for this setting, i.e. shadows on the wall caused by handles blocking some projector's light.

---

![CD58657E-389A-42BF-8E9B-FEDFEF260BF7_1_201_a](https://github.com/user-attachments/assets/e11191af-aac8-40e3-8120-c66d8700cc3c)
*Test image projected onto shared projection area (camera view) of two calibrated projectors.*

![IMG_8022](https://github.com/user-attachments/assets/bc17c2e3-faac-45eb-982d-dd69c6e8ce78)
*Graycode calibration running for one of the projectors.*

![result](https://github.com/user-attachments/assets/da136e6b-4dc3-4fbd-8693-996c68811c69)
*Camera-to-projector pixel mapping computed by decoding the captured graycode patterns (Postprocessing applied).*

![IMG_8025](https://github.com/user-attachments/assets/93a3296a-eaf4-41a6-a539-c8994aeea65f)
***Failing use case:** Projection area for this projector is significantly smaller...*

![result](https://github.com/user-attachments/assets/92da0fce-f647-4b21-aa66-3ca697824520)
*... and the resulting C2P-map fails to be decoded into a homography matrix using `cv::findHomography`.*

---

Simon Hetzer, University of Applied Sciences Bonn-Rhein-Sieg & Osaka University, 2024
