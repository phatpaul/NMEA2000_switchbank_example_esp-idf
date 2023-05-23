// this is written in C++, so we have to add this stuff to make it callable from C:
#ifdef __cplusplus
extern "C" {
#endif

void my_N2K_lib_init(void);
void setSwitchState(int index, bool OnOff);
bool getSwitchState(int index);

#ifdef __cplusplus
}
#endif