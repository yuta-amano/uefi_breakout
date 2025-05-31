#include "graphics.h"
#include "efi.h"
#include "common.h"

#define BALL_WIDTH 16.f
#define BALL_HEIGHT 16.f
#define BALL_IMAGE_BUF_SIZE 1024 // 16px x 16px x 4(bgra)

#define PADDLE_WIDTH 128.f
#define PADDLE_HEIGHT 40.f
#define PADDLE_IMAGE_BUF_SIZE 20480 // 128px x 40px x 4(bgra)

#define BLOCK_WIDTH 90.f
#define BLOCK_HEIGHT 20.f
#define BLOCK_IMAGE_BUF_SIZE 7200 // 90px x 20px x 4(bgra)
#define BLOCK_NUM_X 3
#define BLOCK_NUM_Y 2
#define BLOCK_NUM 6 // 縦5 横6

#define BOUND_EDGE_NUM 3

#define BALL_SPEED 2.f
#define PADDLE_SPEED 8.f

#define GAMECLEAR_IMAGE_WIDTH 420
#define GAMECLEAR_IMAGE_HEIGHT 96
#define GAMECLEAR_IMAGE_BUF_SIZE 161280 // 420px x 96px x 4(bgra)

#define GAMEOVER_IMAGE_WIDTH 376
#define GAMEOVER_IMAGE_HEIGHT 72
#define GAMEOVER_IMAGE_BUF_SIZE 108288 // 420px x 96px x 4(bgra)

#define STATE_GAME 1
#define STATE_CLEAR 2
#define STATE_GAMEOVER 3
#define STATE_WAIT_TITLE 4

unsigned int stage_state = STATE_GAME;

struct LOGIC_RECT {
  float w;
  float h;
  float x;
  float y;
  unsigned char active;
};

struct LOGIC_RECT ball_rect;
unsigned char ball_image[BALL_IMAGE_BUF_SIZE]; 
unsigned short ball_image_name[] = L"ball.bgra";

struct LOGIC_RECT paddle_rect;
unsigned char paddle_image[PADDLE_IMAGE_BUF_SIZE]; 
unsigned short paddle_image_name[] = L"paddle.bgra";

struct LOGIC_RECT blocks[BLOCK_NUM];
unsigned char block_image[BLOCK_IMAGE_BUF_SIZE];

struct LOGIC_RECT edges[BOUND_EDGE_NUM]; // TOP, LEFT, RIGHT

unsigned char gameclear_image[GAMECLEAR_IMAGE_BUF_SIZE];
unsigned short gameclear_image_name[] = L"gameclear.bgra";
unsigned char gameover_image[GAMEOVER_IMAGE_BUF_SIZE];
unsigned short gameover_image_name[] = L"gameover.bgra";

float ball_dx = BALL_SPEED;
float ball_dy = -BALL_SPEED;

void update_game(void);
void check_clear(void);
void create_stage_object(float w, float h, float x, float y, unsigned short image_name[],
  unsigned char *image, struct LOGIC_RECT *rect);
void create_rect(float w, float h, float x, float y, struct LOGIC_RECT *rect);
unsigned int hit_rect(struct LOGIC_RECT a, struct LOGIC_RECT b);
void reflect(struct LOGIC_RECT a, struct LOGIC_RECT b, float *dx, float *dy);
struct RECT convert_logic_rect_to_graphics_rect(struct LOGIC_RECT logic_rect);

void init_stage(void)
{
  stage_state = STATE_GAME;
  ball_dx = BALL_SPEED;
  ball_dy = -BALL_SPEED;
  create_stage_object(BALL_WIDTH, BALL_HEIGHT, (float)GOP->Mode->Info->HorizontalResolution / 2.f - BALL_WIDTH / 2.f, (float)GOP->Mode->Info->VerticalResolution - PADDLE_HEIGHT * 2.f - BALL_HEIGHT, ball_image_name, ball_image, &ball_rect);
  create_stage_object(PADDLE_WIDTH, PADDLE_HEIGHT, (float)GOP->Mode->Info->HorizontalResolution / 2.f - PADDLE_WIDTH / 2.f, (float)GOP->Mode->Info->VerticalResolution - PADDLE_HEIGHT * 2.f, paddle_image_name, paddle_image, &paddle_rect);
  create_rect((float)GOP->Mode->Info->HorizontalResolution, 1.f, 0, 0, &edges[0]); // TOP
  create_rect(1.f, (float)GOP->Mode->Info->VerticalResolution, 0, 0, &edges[1]); // LEFT
  create_rect(1.f, (float)GOP->Mode->Info->VerticalResolution, (float)GOP->Mode->Info->HorizontalResolution - 1.f, 0, &edges[2]); // RIGHT
  // create blocks
  for (int i = 0; i < BLOCK_IMAGE_BUF_SIZE; i++) {
    block_image[i] = 0xff;
  }
  // とりあえず均等に並べておく
  float block_gap_x = ((float)GOP->Mode->Info->HorizontalResolution - BLOCK_NUM_X * BLOCK_WIDTH) / (BLOCK_NUM_X + 1);
  float block_gap_y = BALL_HEIGHT * 2.f;
  for (int i = 0; i < BLOCK_NUM_Y; i++) {
    float block_y = block_gap_y * (i + 1) + BALL_HEIGHT * i;
    for (int j = 0; j < BLOCK_NUM_X; j++) {
      create_rect(BLOCK_WIDTH, BLOCK_HEIGHT, block_gap_x * (j + 1) + BLOCK_WIDTH * j, block_y, &blocks[i * BLOCK_NUM_X + j]);
    }
  }
}

unsigned int update_stage(void)
{
  switch (stage_state) {
    case STATE_GAME:
      update_game();
      break;
    case STATE_CLEAR:
      load_image(GAMECLEAR_IMAGE_WIDTH, GAMECLEAR_IMAGE_HEIGHT, gameclear_image_name, gameclear_image);
      blt(gameclear_image, GAMECLEAR_IMAGE_WIDTH, GAMECLEAR_IMAGE_HEIGHT, (GOP->Mode->Info->HorizontalResolution - GAMECLEAR_IMAGE_WIDTH) / 2, (GOP->Mode->Info->VerticalResolution - GAMECLEAR_IMAGE_HEIGHT) / 2);
      draw_frame_buffer();
      stage_state = STATE_WAIT_TITLE;
      break;
    case STATE_GAMEOVER:
      load_image(GAMEOVER_IMAGE_WIDTH, GAMEOVER_IMAGE_HEIGHT, gameover_image_name, gameover_image);
      blt(gameover_image, GAMEOVER_IMAGE_WIDTH, GAMEOVER_IMAGE_HEIGHT, (GOP->Mode->Info->HorizontalResolution - GAMEOVER_IMAGE_WIDTH) / 2, (GOP->Mode->Info->VerticalResolution - GAMEOVER_IMAGE_HEIGHT) / 2);
      draw_frame_buffer();
      stage_state = STATE_WAIT_TITLE;
      break;
    case STATE_WAIT_TITLE:
      if (getc_no_wait()) {
        return 1;
      }
      break;
  }
  return 0;
}

void update_game()
{
  struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL background = { 0, 0, 0, 0 };
  clear(background);

  switch (getc_no_wait()) {
    case L'a':
      paddle_rect.x -= PADDLE_SPEED;
      if (paddle_rect.x < 0.f) {
        paddle_rect.x = 0.f;
      }
      break;
    case L'd':
      paddle_rect.x += PADDLE_SPEED;
      if (paddle_rect.x + paddle_rect.w > (float)GOP->Mode->Info->HorizontalResolution - 1.f) {
        paddle_rect.x = (float)GOP->Mode->Info->HorizontalResolution - paddle_rect.w - 1.f;
      }
      break;
    default:
      break;
  }

  ball_rect.x += ball_dx;
  ball_rect.y += ball_dy;
  if (ball_rect.y > (paddle_rect.y + PADDLE_HEIGHT + 1)) {
    stage_state = STATE_GAMEOVER;
  }
  if (hit_rect(ball_rect, paddle_rect)) {
    reflect(ball_rect, paddle_rect, &ball_dx, &ball_dy);
  }
  for (unsigned int i = 0; i < BOUND_EDGE_NUM; i++) {
    if (hit_rect(ball_rect, edges[i]) == 1) {
      reflect(ball_rect, edges[i], &ball_dx, &ball_dy);
      break;
    }
  }
  for (unsigned int i = 0; i < BLOCK_NUM; i++) {
    if (!blocks[i].active) {
      continue;
    }
    if (hit_rect(ball_rect, blocks[i])) {
      reflect(ball_rect, blocks[i], &ball_dx, &ball_dy);
      blocks[i].active = 0;
      break;
    }
  }
  check_clear();
  struct RECT ball_pixel_rect = convert_logic_rect_to_graphics_rect(ball_rect);
  struct RECT paddle_pixel_rect = convert_logic_rect_to_graphics_rect(paddle_rect);
  blt(ball_image, ball_pixel_rect.w, ball_pixel_rect.h, ball_pixel_rect.x, ball_pixel_rect.y);
  blt(paddle_image, paddle_pixel_rect.w, paddle_pixel_rect.h, paddle_pixel_rect.x, paddle_pixel_rect.y);
  for (unsigned int i = 0; i < BLOCK_NUM; i++) {
    if (!blocks[i].active) {
      continue;
    }
    struct RECT block_pixel_rect = convert_logic_rect_to_graphics_rect(blocks[i]);
    blt(block_image, block_pixel_rect.w, block_pixel_rect.h, block_pixel_rect.x, block_pixel_rect.y);
  }

  draw_frame_buffer();
}

void check_clear()
{
  if (stage_state != STATE_GAME) {
    return;
  }
  for (unsigned int i = 0; i < BLOCK_NUM; i++) {
    if (blocks[i].active) {
      return;
    }
  }
  stage_state = STATE_CLEAR;
}

void create_stage_object(float w, float h, float x, float y, unsigned short image_name[],
                         unsigned char *image, struct LOGIC_RECT *rect)
{
  load_image(w, h, image_name, image);
  create_rect(w, h, x, y, rect);
}

void create_rect(float w, float h, float x, float y, struct LOGIC_RECT *rect)
{
  rect->w = w;
  rect->h = h;
  rect->x = x;
  rect->y = y;
  rect->active = 1;
}

// 左上基準
unsigned int hit_rect(struct LOGIC_RECT a, struct LOGIC_RECT b)
{
  // 当たっていないケース（x軸）
  // b.x --- b.w    a.x ---- a.w   b.x --- b.w
  if (a.x > (b.x + b.w) || (a.x + a.w) < b.x) {
    return 0;
  }
  // y軸も同様
  if (a.y > (b.y + b.h) || (a.y + a.h) < b.y) {
    return 0;
  }
  // x、y軸とも被っているなら当たっている
  return 1;
}

void reflect(struct LOGIC_RECT a, struct LOGIC_RECT b, float *dx, float *dy)
{
  // 横から当たったとする範囲 = bの中心xからaの中心xの距離がbの幅 / 2 + aの幅 / 2より大きい
  //  b.x --- +b.w / 2 --- +b.w
  //                        a.x --- +a.w / 2 --- +a.w
  //          | ------------------- | <= ここの長さより大きい場合
  if (abs((b.x + b.w / 2.f) - (a.x + a.w / 2.f)) >= a.w / 2.f + b.w / 2.f - 1.f) {
    // 横から当たったので、x方向に反射
    *dx *= -1.f;
    return;
  }
  // それ以外はy方向に反射
  *dy *= -1.f;
}

struct RECT convert_logic_rect_to_graphics_rect(struct LOGIC_RECT logic_rect)
{
  struct RECT graphics_rect;
  graphics_rect.w = float_to_uint(logic_rect.w);
  graphics_rect.h = float_to_uint(logic_rect.h);
  graphics_rect.x = float_to_uint(logic_rect.x);
  graphics_rect.y = float_to_uint(logic_rect.y);
  return graphics_rect;
}
