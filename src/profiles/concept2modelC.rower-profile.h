#pragma once

#include "../utils/enums.h"

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum)
#define DEVICE_NAME Concept2
#define MODEL_NUMBER ModelC

// Hardware settings
// Model C: bobina passiva, 1 impulso per giro del volano
#define IMPULSES_PER_REVOLUTION 1
#define FLYWHEEL_INERTIA 0.1001  // kg·m² — da calibrare con ergometro reale
#define SPROCKET_RADIUS 1.4      // cm   — da verificare con ergometro reale
#define CONCEPT_2_MAGIC_NUMBER 2.8

// Sensor signal filter settings
#define ROTATION_DEBOUNCE_TIME_MIN 10      // ms — bobina passiva, debounce generoso
#define ROWING_STOPPED_THRESHOLD_PERIOD 7'000

// Drag factor filter settings
// I valori 75/250 vengono divisi per 1e6 in settings.model.h
// → corrispondono a k in [75e-6, 250e-6] N·m·s², range tipico Concept2
#define GOODNESS_OF_FIT_THRESHOLD 0.90         // leggermente più permissivo (vs 0.97 genericAir): meno impulsi/giro
#define MAX_DRAG_FACTOR_RECOVERY_PERIOD 6'000
#define LOWER_DRAG_FACTOR_THRESHOLD 75
#define UPPER_DRAG_FACTOR_THRESHOLD 250
#define DRAG_COEFFICIENTS_ARRAY_LENGTH 1       // aggiornamento immediato

// Stroke phase detection filter settings
// Con 1 imp/giro a ~20Hz, ogni impulso dura ~50ms → array da 6 = 300ms di dati
#define MINIMUM_POWERED_TORQUE 0
#define MINIMUM_DRAG_TORQUE 0.07
#define STROKE_DETECTION_TYPE STROKE_DETECTION_TORQUE
#define MINIMUM_RECOVERY_SLOPE 0.01  // Solo se STROKE_DETECTION_TYPE = BOTH o SLOPE
#define MINIMUM_RECOVERY_TIME 800    // ms — uguale a genericAir
#define MINIMUM_DRIVE_TIME 400       // ms — uguale a genericAir
#define IMPULSE_DATA_ARRAY_LENGTH 6
#define DRIVE_HANDLE_FORCES_MAX_CAPACITY 30  // ~1.5s a 20Hz (sufficiente per fase drive)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum)
