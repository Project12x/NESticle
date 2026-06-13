#ifndef NESTICLE_DEVLOG_H
#define NESTICLE_DEVLOG_H

#ifdef __cplusplus
extern "C" {
#endif

extern int nesticle_developer_diagnostics;

#ifdef __cplusplus
}
#endif

#define NESTICLE_DEVLOG_ENABLED (nesticle_developer_diagnostics)

#endif
