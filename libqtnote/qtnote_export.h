#if defined(Q_OS_WIN)
#if !defined(QTNOTE_EXPORT) && !defined(QTNOTE_IMPORT)
#define QTNOTE_EXPORT
#elif defined(QTNOTE_IMPORT)
#if defined(QTNOTE_EXPORT)
#undef QTNOTE_EXPORT
#endif
#define QTNOTE_EXPORT __declspec(dllimport)
#elif defined(QTNOTE_EXPORT)
#undef QTNOTE_EXPORT
#define QTNOTE_EXPORT __declspec(dllexport)
#endif
#else
#define QTNOTE_EXPORT
#endif
