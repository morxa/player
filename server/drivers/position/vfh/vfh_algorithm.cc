#include "vfh_algorithm.h"

#include <math.h>

VFH_Algorithm::VFH_Algorithm( double cell_size,
                              int window_diameter,
                              int sector_angle,
                              double safety_dist,
                              int max_speed,
                              int min_turnrate,
                              int max_turnrate,
                              double free_space_cutoff,
                              double obs_cutoff,
                              double weight_desired_dir,
                              double weight_current_dir )
    : CELL_WIDTH(cell_size),
      WINDOW_DIAMETER(window_diameter),
      SECTOR_ANGLE(sector_angle),
      SAFETY_DIST(safety_dist),
      MAX_SPEED(max_speed),
      MIN_TURNRATE(min_turnrate),
      MAX_TURNRATE(max_turnrate),
      Binary_Hist_Low(free_space_cutoff),
      Binary_Hist_High(obs_cutoff),
      U1(weight_desired_dir),
      U2(weight_current_dir),
      Desired_Angle(90),
      Picked_Angle(90),
      Last_Picked_Angle(Picked_Angle)

{
}

VFH_Algorithm::~VFH_Algorithm()
{
}

int VFH_Algorithm::Init()
{
  int x, y, i;
  float plus_dir, neg_dir, plus_sector, neg_sector;
  bool plus_dir_bw, neg_dir_bw, dir_around_sector;
  float neg_sector_to_neg_dir, neg_sector_to_plus_dir;
  float plus_sector_to_neg_dir, plus_sector_to_plus_dir;

  Goal_Behind = 0;

  /*
  CELL_WIDTH = cell_width;
  WINDOW_DIAMETER = window_diameter;
  SECTOR_ANGLE = sector_angle;
  ROBOT_RADIUS = robot_radius;
  SAFETY_DIST = safety_dist;
  */

  CENTER_X = (int)floor(WINDOW_DIAMETER / 2.0);
  CENTER_Y = CENTER_X;
  HIST_SIZE = (int)rint(360.0 / SECTOR_ANGLE);

  /*
  MAX_SPEED = max_speed;
  MAX_TURNRATE = max_turnrate;
  MIN_TURNRATE = min_turnrate;

  Binary_Hist_Low = binary_hist_low;
  Binary_Hist_High = binary_hist_high;

  U1 = u1;
  U2 = u2;
  */

  // it works now; let's leave the verbose debug statement out
  /*
  printf("CELL_WIDTH: %1.1f\tWINDOW_DIAMETER: %d\tSECTOR_ANGLE: %d\tROBOT_RADIUS: %1.1f\tSAFETY_DIST: %1.1f\tMAX_SPEED: %d\tMAX_TURNRATE: %d\tFree Space Cutoff: %1.1f\tObs Cutoff: %1.1f\tWeight Desired Dir: %1.1f\tWeight Current_Dir:%1.1f\n", CELL_WIDTH, WINDOW_DIAMETER, SECTOR_ANGLE, ROBOT_RADIUS, SAFETY_DIST, MAX_SPEED, MAX_TURNRATE, Binary_Hist_Low, Binary_Hist_High, U1, U2);
  */

  VFH_Allocate();

  for(x=0;x<HIST_SIZE;x++) {
    Hist[x] = 0;
    Last_Binary_Hist[x] = 1;
  }

  for(x=0;x<=MAX_SPEED;x++) {
//    Min_Turning_Radius[x] = (int)rint(150.0 * x / 200.0);
    Min_Turning_Radius[x] = 150;
  }

  for(x=0;x<WINDOW_DIAMETER;x++) {
    for(y=0;y<WINDOW_DIAMETER;y++) {
      Cell_Mag[x][y] = 0;
      Cell_Dist[x][y] = sqrt(pow((CENTER_X - x), 2) + pow((CENTER_Y - y), 2)) * CELL_WIDTH;

      Cell_Base_Mag[x][y] = pow((3000.0 - Cell_Dist[x][y]), 4) / 100000000.0;

      if (x < CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(CENTER_Y - y) / (float)(CENTER_X - x));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 180.0 - Cell_Direction[x][y];
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = 180.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(y - CENTER_Y) / (float)(CENTER_X - x));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 180.0 + Cell_Direction[x][y];
        }
      } else if (x == CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = 90.0;
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = -1.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = 270.0;
        }
      } else if (x > CENTER_X) {
        if (y < CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(CENTER_Y - y) / (float)(x - CENTER_X));
          Cell_Direction[x][y] *= (360.0 / 6.28);
        } else if (y == CENTER_Y) {
          Cell_Direction[x][y] = 0.0;
        } else if (y > CENTER_Y) {
          Cell_Direction[x][y] = atan((float)(y - CENTER_Y) / (float)(x - CENTER_X));
          Cell_Direction[x][y] *= (360.0 / 6.28);
          Cell_Direction[x][y] = 360.0 - Cell_Direction[x][y];
        }
      }

      if (Cell_Dist[x][y] > 0)
        Cell_Enlarge[x][y] = (float)atan((ROBOT_RADIUS + SAFETY_DIST) / Cell_Dist[x][y]) * 
		(360.0 / 6.28);
      else
        Cell_Enlarge[x][y] = 0;

      Cell_Sector[x][y].clear();
      for(i=0;i<(360 / SECTOR_ANGLE);i++) {
        plus_dir = Cell_Direction[x][y] + Cell_Enlarge[x][y];
        neg_dir = Cell_Direction[x][y] - Cell_Enlarge[x][y];
        plus_sector = (i + 1) * (float)SECTOR_ANGLE;
        neg_sector = i * (float)SECTOR_ANGLE;

        if ((neg_sector - neg_dir) > 180) {
          neg_sector_to_neg_dir = neg_dir - (neg_sector - 360);
        } else {
          if ((neg_dir - neg_sector) > 180) {
            neg_sector_to_neg_dir = neg_sector - (neg_dir + 360);
          } else {
            neg_sector_to_neg_dir = neg_dir - neg_sector;
          }
        }

        if ((plus_sector - neg_dir) > 180) {
          plus_sector_to_neg_dir = neg_dir - (plus_sector - 360);
        } else {
          if ((neg_dir - plus_sector) > 180) {
            plus_sector_to_neg_dir = plus_sector - (neg_dir + 360);
          } else {
            plus_sector_to_neg_dir = neg_dir - plus_sector;
          }
        }

        if ((plus_sector - plus_dir) > 180) {
          plus_sector_to_plus_dir = plus_dir - (plus_sector - 360);
        } else {
          if ((plus_dir - plus_sector) > 180) {
            plus_sector_to_plus_dir = plus_sector - (plus_dir + 360);
          } else {
            plus_sector_to_plus_dir = plus_dir - plus_sector;
          }
        }

        if ((neg_sector - plus_dir) > 180) {
          neg_sector_to_plus_dir = plus_dir - (neg_sector - 360);
        } else {
          if ((plus_dir - neg_sector) > 180) {
            neg_sector_to_plus_dir = neg_sector - (plus_dir + 360);
          } else {
            neg_sector_to_plus_dir = plus_dir - neg_sector;
          }
        }

        plus_dir_bw = 0;
        neg_dir_bw = 0;
        dir_around_sector = 0;

        if ((neg_sector_to_neg_dir >= 0) && (plus_sector_to_neg_dir <= 0)) {
          neg_dir_bw = 1; 
        }

        if ((neg_sector_to_plus_dir >= 0) && (plus_sector_to_plus_dir <= 0)) {
          plus_dir_bw = 1; 
        }

        if ((neg_sector_to_neg_dir <= 0) && (neg_sector_to_plus_dir >= 0)) {
          dir_around_sector = 1; 
        }

        if ((plus_sector_to_neg_dir <= 0) && (plus_sector_to_plus_dir >= 0)) {
          plus_dir_bw = 1; 
        }

        if ((plus_dir_bw) || (neg_dir_bw) || (dir_around_sector)) {
          Cell_Sector[x][y].push_back(i);
        }
      }
    }
  }
  return(1);
}

int VFH_Algorithm::VFH_Allocate() 
{
  std::vector<float> temp_vec;
  std::vector<int> temp_vec3;
  std::vector<std::vector<int> > temp_vec2;
  int x;

  Cell_Direction.clear();
  Cell_Base_Mag.clear();
  Cell_Mag.clear();
  Cell_Dist.clear();
  Cell_Enlarge.clear();
  Cell_Sector.clear();

  temp_vec.clear();
  for(x=0;x<WINDOW_DIAMETER;x++) {
    temp_vec.push_back(0);
  }

  temp_vec2.clear();
  temp_vec3.clear();
  for(x=0;x<WINDOW_DIAMETER;x++) {
    temp_vec2.push_back(temp_vec3);
  }

  for(x=0;x<WINDOW_DIAMETER;x++) {
    Cell_Direction.push_back(temp_vec);
    Cell_Base_Mag.push_back(temp_vec);
    Cell_Mag.push_back(temp_vec);
    Cell_Dist.push_back(temp_vec);
    Cell_Enlarge.push_back(temp_vec);
    Cell_Sector.push_back(temp_vec2);
  }

  Hist = new float[HIST_SIZE];
  Last_Binary_Hist = new float[HIST_SIZE];
  Min_Turning_Radius = new int[MAX_SPEED+1];

  return(1);
}

int VFH_Algorithm::Update_VFH( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2], int &speed, int &turnrate ) 
{
  int print = 0;

/*
  if ((Goal_Behind == 1) || (Desired_Angle > 180)) {
//  if (Desired_Angle > 90 && Desired_Angle < 270) {
    speed = 1;
    Goal_Behind = 1;
    Set_Motion();
    return(1);
  }
  */

  Build_Primary_Polar_Histogram(laser_ranges);
  if (print) {
    printf("Primary Histogram\n");
    Print_Hist();
  }

  Build_Binary_Polar_Histogram();
  if (print) {
    printf("Binary Histogram\n");
    Print_Hist();
  }

  speed += 20;
  if (speed > MAX_SPEED) {
    speed = MAX_SPEED;
  }

  Build_Masked_Polar_Histogram(speed);
  if (print) {
    printf("Masked Histogram\n");
    Print_Hist();
  }

  Select_Direction();

//  printf("Picked Angle: %f\n", Picked_Angle);

  if (Picked_Angle != -9999) {
      Set_Motion( speed, turnrate );
  } else {
    speed = 0;
    Set_Motion( speed, turnrate );
    return(1);
  }

  if (print)
    printf("SPEED: %d\t TURNRATE: %d\n", speed, turnrate);

  return(1);
}


float VFH_Algorithm::Delta_Angle(int a1, int a2) 
{
  return(Delta_Angle((float)a1, (float)a2));
}

float VFH_Algorithm::Delta_Angle(float a1, float a2) 
{
  float diff;

  diff = a2 - a1;

  if (diff > 180) {
    diff -= 360;
  } else if (diff < -180) {
    diff += 360;
  }

  return(diff);
}


int VFH_Algorithm::Bisect_Angle(int angle1, int angle2) 
{
  float a;
  int angle;

  a = Delta_Angle((float)angle1, (float)angle2);

  angle = (int)rint(angle1 + (a / 2.0));
  if (angle < 0) {
    angle += 360;
  } else if (angle >= 360) {
    angle -= 360;
  }

  return(angle);
}


int VFH_Algorithm::Select_Candidate_Angle() 
{
  unsigned int i;
  float weight, min_weight;

  if (Candidate_Angle.size() == 0) {
    Picked_Angle = -9999;
    return(1);
  }

  Picked_Angle = 90;
  min_weight = 10000000;
  for(i=0;i<Candidate_Angle.size();i++) {
//printf("CANDIDATE: %f\n", Candidate_Angle[i]);
    weight = U1 * fabs(Delta_Angle(Desired_Angle, Candidate_Angle[i])) +
    	U2 * fabs(Delta_Angle(Last_Picked_Angle, Candidate_Angle[i]));
    if (weight < min_weight) {
      min_weight = weight;
      Picked_Angle = Candidate_Angle[i];
    }
  }

  Last_Picked_Angle = Picked_Angle;

  return(1);
}


int VFH_Algorithm::Select_Direction() 
{
  int start, i, left;
  float angle, new_angle;
  std::vector<std::pair<int,int> > border;
  std::pair<int,int> new_border;

  Candidate_Angle.clear();

  start = -1; 
  for(i=0;i<HIST_SIZE;i++) {
    if (Hist[i] == 1) {
      start = i;
      break;
    }
  }

  if (start == -1) {
    Candidate_Angle.push_back(Desired_Angle);
    return(1);
  }

  border.clear();

//printf("Start: %d\n", start);
  left = 1;
  for(i=start;i<=(start+HIST_SIZE);i++) {
    if ((Hist[i % HIST_SIZE] == 0) && (left)) {
      new_border.first = (i % HIST_SIZE) * SECTOR_ANGLE;
      left = 0;
    }

    if ((Hist[i % HIST_SIZE] == 1) && (!left)) {
      new_border.second = ((i % HIST_SIZE) - 1) * SECTOR_ANGLE;
      if (new_border.second < 0) {
        new_border.second += 360;
      }
      border.push_back(new_border);
      left = 1;
    }
  }

  for(i=0;i<(int)border.size();i++) {
//printf("BORDER: %f %f\n", border[i].first, border[i].second);
    angle = Delta_Angle(border[i].first, border[i].second);

    if (fabs(angle) < 10) {
      continue;
    }

    if (fabs(angle) < 80) {
      new_angle = border[i].first + (border[i].second - border[i].first) / 2.0;

      Candidate_Angle.push_back(new_angle);
    } else {
      new_angle = border[i].first + (border[i].second - border[i].first) / 2.0;

      Candidate_Angle.push_back(new_angle);

      new_angle = (float)((border[i].first + 40) % 360);
      Candidate_Angle.push_back(new_angle);

      new_angle = (float)(border[i].second - 40);
      if (new_angle < 0) 
        new_angle += 360;
      Candidate_Angle.push_back(new_angle);

      // See if candidate dir is in this opening
      if ((Delta_Angle(Desired_Angle, Candidate_Angle[Candidate_Angle.size()-2]) < 0) && 
		(Delta_Angle(Desired_Angle, Candidate_Angle[Candidate_Angle.size()-1]) > 0)) {
        Candidate_Angle.push_back(Desired_Angle);
      }
    }
  }

  Select_Candidate_Angle();

  return(1);
}

void VFH_Algorithm::Print_Cells_Dir() 
{
  int x, y;

  printf("\nCell Directions:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Direction[x][y]);
    }
    printf("\n");
  }
}

void VFH_Algorithm::Print_Cells_Mag() 
{
  int x, y;

  printf("\nCell Magnitudes:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Mag[x][y]);
    }
    printf("\n");
  }
}

void VFH_Algorithm::Print_Cells_Dist() 
{
  int x, y;

  printf("\nCell Distances:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Dist[x][y]);
    }
    printf("\n");
  }
}

void VFH_Algorithm::Print_Cells_Sector() 
{
  int x, y;
  unsigned int i;

  printf("\nCell Sectors:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      for(i=0;i<Cell_Sector[x][y].size();i++) {
        if (i < (Cell_Sector[x][y].size() -1 )) {
          printf("%d,", Cell_Sector[x][y][i]);
        } else {
          printf("%d\t", Cell_Sector[x][y][i]);
        }
      }
    }
    printf("\n");
  }
}

void VFH_Algorithm::Print_Cells_Enlargement_Angle() 
{
  int x, y;

  printf("\nEnlargement Angles:\n");
  printf("****************\n");
  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      printf("%1.1f\t", Cell_Enlarge[x][y]);
    }
    printf("\n");
  }
}

void VFH_Algorithm::Print_Hist() 
{
  int x;
  printf("Histogram:\n");
  printf("****************\n");

  for(x=0;x<=(HIST_SIZE/2);x++) {
    printf("%d:\t%1.1f\n", (x * SECTOR_ANGLE), Hist[x]);
  }
  printf("\n\n");
}

int VFH_Algorithm::Calculate_Cells_Mag( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2] ) 
{
  int x, y;

/*
printf("Laser Ranges\n");
printf("************\n");
for(x=0;x<=360;x++) {
   printf("%d: %f\n", x, this->laser_ranges[x][0]);
}
*/

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      if (Cell_Direction[x][y] >= 180) {	// behind the robot, so cant sense, assume free
        Cell_Mag[x][y] = 0.0;
      } else {
        if ((Cell_Dist[x][y] + CELL_WIDTH / 2.0) > laser_ranges[(int)rint(Cell_Direction[x][y] * 2.0)][0]) {
          Cell_Mag[x][y] = Cell_Base_Mag[x][y];
        } else {
          Cell_Mag[x][y] = 0.0;
        }
      }
    }
  }

  return(1);
}

int VFH_Algorithm::Build_Primary_Polar_Histogram( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2]) 
{
  int x, y;
  unsigned int i;

  for(x=0;x<HIST_SIZE;x++) {
    Hist[x] = 0;
  }

  Calculate_Cells_Mag( laser_ranges );

//  Print_Cells_Dist();
//  Print_Cells_Dir();
//  Print_Cells_Mag();
//  Print_Cells_Sector();
//  Print_Cells_Enlargement_Angle();

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      for(i=0;i<Cell_Sector[x][y].size();i++) {
        Hist[Cell_Sector[x][y][i]] += Cell_Mag[x][y];
      }
    }
  }

  return(1);
}

int VFH_Algorithm::Build_Binary_Polar_Histogram() 
{
  int x;

  for(x=0;x<HIST_SIZE;x++) {
    if (Hist[x] > Binary_Hist_High) {
      Hist[x] = 1.0;
    } else if (Hist[x] < Binary_Hist_Low) {
      Hist[x] = 0.0;
    } else {
      Hist[x] = Last_Binary_Hist[x];
    }
  }

  for(x=0;x<HIST_SIZE;x++) {
    Last_Binary_Hist[x] = Hist[x];
  }

  return(1);
}

int VFH_Algorithm::Build_Masked_Polar_Histogram(int speed) 
{
  int x, y;
  float center_x_right, center_x_left, center_y, dist_r, dist_l;
  float theta, phi_b, phi_l, phi_r, total_dist, angle;

  center_x_right = CENTER_X + (Min_Turning_Radius[speed] / (float)CELL_WIDTH);
  center_x_left = CENTER_X - (Min_Turning_Radius[speed] / (float)CELL_WIDTH);
  center_y = CENTER_Y;

  theta = 90;
  phi_b = theta + 180;
  phi_l = phi_b;
  phi_r = phi_b;

  phi_l = 180;
  phi_r = 0;

  if (speed > 0) {
    total_dist = Min_Turning_Radius[speed] + ROBOT_RADIUS + SAFETY_DIST;
  } else {
    total_dist = Min_Turning_Radius[speed] + ROBOT_RADIUS;
  }

  for(y=0;y<WINDOW_DIAMETER;y++) {
    for(x=0;x<WINDOW_DIAMETER;x++) {
      if (Cell_Mag[x][y] == 0) 
        continue;

      if ((Delta_Angle(Cell_Direction[x][y], theta) > 0) && 
		(Delta_Angle(Cell_Direction[x][y], phi_r) <= 0)) {
        dist_r = sqrt(pow((center_x_right - x), 2) + pow((center_y - y), 2)) * CELL_WIDTH;
        if (dist_r < total_dist) { 
          phi_r = Cell_Direction[x][y];
        }
      } else {
        if ((Delta_Angle(Cell_Direction[x][y], theta) <= 0) && 
		(Delta_Angle(Cell_Direction[x][y], phi_l) > 0)) {
          dist_l = sqrt(pow((center_x_left - x), 2) + pow((center_y - y), 2)) * CELL_WIDTH;
          if (dist_l < total_dist) { 
            phi_l = Cell_Direction[x][y];
          }
        }
      }
    }
  }

  for(x=0;x<HIST_SIZE;x++) {
    angle = x * SECTOR_ANGLE;
    if ((Hist[x] == 0) && (((Delta_Angle((float)angle, phi_r) <= 0) && 
	(Delta_Angle((float)angle, theta) >= 0)) || 
      	((Delta_Angle((float)angle, phi_l) >= 0) &&
	(Delta_Angle((float)angle, theta) <= 0)))) {
      Hist[x] = 0;
    } else {
      Hist[x] = 1;
    }
  }

  return(1);
}


int VFH_Algorithm::Set_Motion( int &speed, int &turnrate ) 
{
  //int i;

  // This happens if all directions blocked, so just spin in place
  if (speed <= 0) {
    //printf("stop\n");
    turnrate = MAX_TURNRATE;
    speed = 0;
  }
/*
  // goal behind robot, turn toward it
  else if (speed == 1) { 
    //printf("turn %f\n", Desired_Angle);
    speed = 0;
    if ((Desired_Angle > 270) && (Desired_Angle < 60)) {
      turnrate = -40;
    } else if ((Desired_Angle > 120) && (Desired_Angle < 270)) {
      turnrate = 40;
    } else {
      turnrate = MAX_TURNRATE;
    }
  }
*/
  else {
    //printf("Picked %f\n", Picked_Angle);
    if ((Picked_Angle > 270) && (Picked_Angle < 360)) {
      turnrate = -1 * MAX_TURNRATE;
    } else if ((Picked_Angle < 270) && (Picked_Angle > 180)) {
      turnrate = MAX_TURNRATE;
    } else {
      turnrate = (int)rint(((float)(Picked_Angle - 90) / 75.0) * MAX_TURNRATE);

      if (turnrate > MAX_TURNRATE) {
        turnrate = MAX_TURNRATE;
      } else if (turnrate < (-1 * MAX_TURNRATE)) {
        turnrate = -1 * MAX_TURNRATE;
      }

//      if (abs(turnrate) > (0.9 * MAX_TURNRATE)) {
//        speed = 0;
//      }
    }
  }

//  speed and turnrate have been set for the calling function -- return.

  return(1);
}
