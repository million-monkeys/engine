#ifdef DOCTEST_CONFIG_DISABLE

    int game_main (int argc, char** argv);

    int main (int argc, char** argv) {
        return game_main(argc, argv);
    }

#else

    #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    #include <doctest.h>

#endif
