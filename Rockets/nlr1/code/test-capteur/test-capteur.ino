#include "ICM42688.h"

#define LED_CARTE PB2
#define GYRO_CS PA4

ICM42688 IMU(SPI, GYRO_CS);

// --- ALIGNEMENT DE LA CARTE (BOARD ALIGNMENT) ---
// Règle tes angles ici en degrés (comme dans l'onglet Configuration de Betaflight).
// Si ta carte est à la verticale, modifie le PITCH à 90.0 ou -90.0, ou le ROLL.
float ALIGN_ROLL  = 0.0f;
float ALIGN_PITCH = 90.0f; // Exemple : Bascule la carte de 90°
float ALIGN_YAW   = 0.0f;

float rMat[3][3]; // Matrice de rotation précalculée

// Biais de calibration
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;

// --- FILTRES PT1 BETAFLIGHT ---
struct PT1Filter {
  float state;
  float k;
  void computeK(float hz, float dt) {
    float rc = 1.0f / (2.0f * PI * hz);
    k = dt / (rc + dt);
  }
  float apply(float input) {
    state = state + k * (input - state);
    return state;
  }
};

PT1Filter accelFilterX, accelFilterY, accelFilterZ;
PT1Filter gyroFilterX, gyroFilterY, gyroFilterZ;

// Fréquence de coupure des filtres en Hz
const float ACCEL_CUTOFF_HZ = 15.0f; 
const float GYRO_CUTOFF_HZ = 40.0f;  

// --- MAHONY ---
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
float Kp = 1.5f;

float angleX = 0.0f; // Roll
float angleY = 0.0f; // Pitch
float angleZ = 0.0f; // Yaw

unsigned long tempsPrecedent = 0;

// ==========================================
// BETAFLIGHT : Génération de la matrice
// ==========================================
void buildRotationMatrix(float roll, float pitch, float yaw) {
  float cosR = cos(radians(roll)), sinR = sin(radians(roll));
  float cosP = cos(radians(pitch)), sinP = sin(radians(pitch));
  float cosY = cos(radians(yaw)), sinY = sin(radians(yaw));

  rMat[0][0] = cosP * cosY;
  rMat[0][1] = cosR * sinY + sinR * sinP * cosY;
  rMat[0][2] = sinR * sinY - cosR * sinP * cosY;

  rMat[1][0] = -cosP * sinY;
  rMat[1][1] = cosR * cosY - sinR * sinP * sinY;
  rMat[1][2] = sinR * cosY + cosR * sinP * sinY;

  rMat[2][0] = sinP;
  rMat[2][1] = -sinR * cosP;
  rMat[2][2] = cosR * cosP;
}

// BETAFLIGHT : Application de l'alignement
void alignSensor(float &x, float &y, float &z) {
  float rx = x * rMat[0][0] + y * rMat[0][1] + z * rMat[0][2];
  float ry = x * rMat[1][0] + y * rMat[1][1] + z * rMat[1][2];
  float rz = x * rMat[2][0] + y * rMat[2][1] + z * rMat[2][2];
  x = rx; y = ry; z = rz;
}

// ==========================================
// MAHONY CORE
// ==========================================
void updateMahony(float gx, float gy, float gz, float ax, float ay, float az, float dt, float current_Kp) {
  float recipNorm, halfvx, halfvy, halfvz, halfex, halfey, halfez, qa, qb, qc;

  gx *= 0.0174533f; gy *= 0.0174533f; gz *= 0.0174533f;

  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
    recipNorm = 1.0f / sqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm; ay *= recipNorm; az *= recipNorm;

    halfvx = q1 * q3 - q0 * q2;
    halfvy = q0 * q1 + q2 * q3;
    halfvz = q0 * q0 - 0.5f + q3 * q3;

    halfex = (ay * halfvz - az * halfvy);
    halfey = (az * halfvx - ax * halfvz);
    halfez = (ax * halfvy - ay * halfvx);

    gx += current_Kp * halfex;
    gy += current_Kp * halfey;
    gz += current_Kp * halfez;
  }

  gx *= (0.5f * dt); gy *= (0.5f * dt); gz *= (0.5f * dt);
  qa = q0; qb = q1; qc = q2;
  q0 += (-qb * gx - qc * gy - q3 * gz);
  q1 += (qa * gx + qc * gz - q3 * gy);
  q2 += (qa * gy - qb * gz + q3 * gx);
  q3 += (qa * gz + qb * gy - qc * gx);

  recipNorm = 1.0f / sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
  q0 *= recipNorm; q1 *= recipNorm; q2 *= recipNorm; q3 *= recipNorm;
}

void calibrerGyro() {
  Serial.println("[CALIBRATION GYRO] Ne pas bouger...");
  float gX = 0, gY = 0, gZ = 0;
  for (int i = 0; i < 500; i++) {
    IMU.getAGT();
    gX += IMU.gyrX(); gY += IMU.gyrY(); gZ += IMU.gyrZ();
    delay(4);
  }
  gyroBiasX = gX / 500.0f; gyroBiasY = gY / 500.0f; gyroBiasZ = gZ / 500.0f;
  Serial.println("Calibration OK");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (IMU.begin() < 0) {
    Serial.println("Erreur IMU");
    while (1) {}
  }

  IMU.setAccelFS(ICM42688::gpm16);   
  IMU.setGyroFS(ICM42688::dps2000);  
  IMU.setAccelODR(ICM42688::odr100);
  IMU.setGyroODR(ICM42688::odr100);
  IMU.setFilters(true, true);

  // Précalculer la matrice d'alignement au démarrage
  buildRotationMatrix(ALIGN_ROLL, ALIGN_PITCH, ALIGN_YAW);
  
  calibrerGyro();
  tempsPrecedent = micros();
}

void loop() {
  unsigned long tempsActuel = micros();
  
  if (tempsActuel - tempsPrecedent >= 10000) { // Boucle à 100 Hz
    float dt = (tempsActuel - tempsPrecedent) / 1000000.0f;
    tempsPrecedent = tempsActuel;

    IMU.getAGT();

    // 1. Lecture brute
    float raw_ax = IMU.accX();
    float raw_ay = IMU.accY();
    float raw_az = IMU.accZ();
    float raw_gx = IMU.gyrX() - gyroBiasX;
    float raw_gy = IMU.gyrY() - gyroBiasY;
    float raw_gz = IMU.gyrZ() - gyroBiasZ;

    // 2. ALIGNEMENT EXACT (Matrice 3D)
    alignSensor(raw_ax, raw_ay, raw_az);
    alignSensor(raw_gx, raw_gy, raw_gz);

    // 3. FILTRAGE PT1 BETAFLIGHT
    accelFilterX.computeK(ACCEL_CUTOFF_HZ, dt);
    accelFilterY.computeK(ACCEL_CUTOFF_HZ, dt);
    accelFilterZ.computeK(ACCEL_CUTOFF_HZ, dt);
    gyroFilterX.computeK(GYRO_CUTOFF_HZ, dt);
    gyroFilterY.computeK(GYRO_CUTOFF_HZ, dt);
    gyroFilterZ.computeK(GYRO_CUTOFF_HZ, dt);

    float ax = accelFilterX.apply(raw_ax);
    float ay = accelFilterY.apply(raw_ay);
    float az = accelFilterZ.apply(raw_az);
    float gx = gyroFilterX.apply(raw_gx);
    float gy = gyroFilterY.apply(raw_gy);
    float gz = gyroFilterZ.apply(raw_gz);

    // 4. COUPURE ACCELÉROMÈTRE PENDANT LA POUSSÉE
    float forceG = sqrt(ax * ax + ay * ay + az * az);
    float current_Kp = (forceG > 1.4f || forceG < 0.6f) ? 0.0f : Kp;

    // 5. MAHONY UPDATE
    updateMahony(gx, gy, gz, ax, ay, az, dt, current_Kp);

    // 6. EULER ANGLES
    angleX = atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * 57.29578f;
    angleY = asin(2.0f * (q0 * q2 - q3 * q1)) * 57.29578f;
    angleZ = atan2(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * 57.29578f;

    Serial.print("X(Roll): ");  Serial.print(angleX, 1); Serial.print("\t");
    Serial.print("Y(Pitch): "); Serial.print(angleY, 1); Serial.print("\t");
    Serial.print("Z(Yaw): ");   Serial.println(angleZ, 1);
  }
}