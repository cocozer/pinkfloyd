#include "application_ui.h"
#include "SDL2_gfxPrimitives.h"
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <algorithm>
#include <iostream>
using namespace std;

#define EPSILON 0.0001f

struct Coords
{
    int x, y;

    bool operator==(const Coords& other)
    {
        return x == other.x and y == other.y;
    }
};

struct Segment
{
    Coords p1, p2;
    Segment(Coords point1, Coords point2) : p1(point1), p2(point2) {} //constructeur : initialise les membres p1 et p2 avec les points point1 et point2 lorsqu'un objet Segment est créé
};

struct Triangle
{
    Coords p1, p2, p3;
    bool complet=false;

    Triangle(const Coords& _p1, const Coords& _p2, const Coords& _p3, bool _complet=false) : p1(_p1), p2(_p2), p3(_p3), complet(_complet) {}

    inline bool operator==(const Triangle& valueToCompare){
        if ((this->p1)==(valueToCompare.p1)){
            if((this->p2)==(valueToCompare.p2)){
                if((this->p3)==(valueToCompare.p3)){
                    return true;}}}
        return false;
    }
};

struct Application
{
    int width, height;
    Coords focus{100, 100};

    std::vector<Coords> points;
    std::vector<Triangle> triangles;
    std::vector<Segment> segments;
};

bool compareCoords(Coords point1, Coords point2)
{
    if (point1.y == point2.y)
        return point1.x < point2.x;
    return point1.y < point2.y;
}

void drawPoints(SDL_Renderer *renderer, const std::vector<Coords> &points)
{
    for (std::size_t i = 0; i < points.size(); i++)
    {
        filledCircleRGBA(renderer, points[i].x, points[i].y, 3, 240, 240, 23, SDL_ALPHA_OPAQUE);
    }
}

void drawSegments(SDL_Renderer *renderer, const std::vector<Segment> &segments)
{
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        lineRGBA(
            renderer,
            segments[i].p1.x, segments[i].p1.y,
            segments[i].p2.x, segments[i].p2.y,
            240, 240, 20, SDL_ALPHA_OPAQUE);
    }
}

void drawTriangles(SDL_Renderer *renderer, const std::vector<Triangle> &triangles)
{
    for (std::size_t i = 0; i < triangles.size(); i++)
    {
        const Triangle& t = triangles[i];
        trigonRGBA(
            renderer,
            t.p1.x, t.p1.y,
            t.p2.x, t.p2.y,
            t.p3.x, t.p3.y,
            0, 240, 160, SDL_ALPHA_OPAQUE
        );
    }
}

void draw(SDL_Renderer *renderer, const Application &app)
{
    /* Remplissez cette fonction pour faire l'affichage du jeu */
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    drawPoints(renderer, app.points);
    drawTriangles(renderer, app.triangles);
    drawSegments(renderer, app.segments);
}

/*
   Détermine si un point se trouve dans un cercle définit par trois points
   Retourne, par les paramètres, le centre et le rayon
*/
bool CircumCircle(
    float pX, float pY,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float *xc, float *yc, float *rsqr
)
{
    float m1, m2, mx1, mx2, my1, my2;
    float dx, dy, drsqr;
    float fabsy1y2 = fabs(y1 - y2);
    float fabsy2y3 = fabs(y2 - y3);

    /* Check for coincident points */
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return (false);

    if (fabsy1y2 < EPSILON)
    {
        m2 = -(x3 - x2) / (y3 - y2);
        mx2 = (x2 + x3) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (x2 + x1) / 2.0;
        *yc = m2 * (*xc - mx2) + my2;
    }
    else if (fabsy2y3 < EPSILON)
    {
        m1 = -(x2 - x1) / (y2 - y1);
        mx1 = (x1 + x2) / 2.0;
        my1 = (y1 + y2) / 2.0;
        *xc = (x3 + x2) / 2.0;
        *yc = m1 * (*xc - mx1) + my1;
    }
    else
    {
        m1 = -(x2 - x1) / (y2 - y1);
        m2 = -(x3 - x2) / (y3 - y2);
        mx1 = (x1 + x2) / 2.0;
        mx2 = (x2 + x3) / 2.0;
        my1 = (y1 + y2) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
        {
            *yc = m1 * (*xc - mx1) + my1;
        }
        else
        {
            *yc = m2 * (*xc - mx2) + my2;
        }
    }

    dx = x2 - *xc;
    dy = y2 - *yc;
    *rsqr = dx * dx + dy * dy;

    dx = pX - *xc;
    dy = pY - *yc;
    drsqr = dx * dx + dy * dy;

    return ((drsqr - *rsqr) <= EPSILON ? true : false);
}

void fillVectorTriangles(Application &app) // Fonction qui ajoute les points aux triangles à partir des derniers points ajoutés
{
    if(app.points.size()%3 == 0) {
        int indice1 = app.points.size()-3;
        int indice2 = app.points.size()-2;
        int indice3 = app.points.size()-1;

        app.triangles.push_back(Triangle{app.points[indice1], app.points[indice2], app.points[indice3]});
    }
}

void TriangulationDelaunay(Application &app)
{
    sort(app.points.begin(), app.points.end(), [](Coords &p1, Coords &p2) { return p1.x < p2.x; }); // On trie les points par la coordonnée x

    app.triangles.clear(); // On vide les triangles
    app.triangles.emplace_back(Triangle{Coords{-1000, -1000}, Coords{500, 3000}, Coords{1500, -1000}}); // On créé un très grand triangle

    std::vector<Segment> LS; // On crée un vector LS contenant des segments
    for (auto &p : app.points) { // Pour chaque point p du repère
        LS.clear();
        std::vector<Triangle> trianglesToRemove; // On stocke les triangles à supprimer

        for (auto &T : app.triangles) { // Pour chaque triangle T déjà créé
            float xc; // Coordonnée x du centre du cercle circonscrit
            float yc; // Coordonnée y du centre du cercle circonscrit
            float rsqr; // Carré du rayon du cercle circonscrit

            if (CircumCircle(p.x, p.y, T.p1.x, T.p1.y, T.p2.x, T.p2.y, T.p3.x, T.p3.y, &xc, &yc, &rsqr)) {
                // Récupérer les différents segments de ce triangle dans LS
                LS.emplace_back(T.p1, T.p2);
                LS.emplace_back(T.p2, T.p3);
                LS.emplace_back(T.p3, T.p1);
                trianglesToRemove.push_back(T);
            }
        }

        std::vector<Segment> uniqueSegments;
        for (auto &S : LS) { // Pour chaque segment de la liste
            // Si un segment est un doublon d'un autre, les supprimer
            bool isUnique = true;
            for (auto it = uniqueSegments.begin(); it != uniqueSegments.end();) {
                if ((it->p1 == S.p1) && (it->p2 == S.p2) || (it->p1 == S.p2) && (it->p2 == S.p1)) {
                    it = uniqueSegments.erase(it);
                    isUnique = false;
                } else {
                    ++it;
                }
            }
            if (isUnique) {
                uniqueSegments.emplace_back(S);
            }
        }

        for (auto &T : trianglesToRemove) {
            auto it = find(app.triangles.begin(), app.triangles.end(), T);
            if (it != app.triangles.end()) {
                app.triangles.erase(it);
            }
        }

        // Pour chaque segment S de la liste LS, créer un nouveau triangle composé du segment S et du point P
        for (auto &S : uniqueSegments) {
            Triangle newTriangle(p, S.p1, S.p2);
            app.triangles.emplace_back(newTriangle);
        }
    }
}




void construitVoronoi(Application &app)
{
    // On réinitialise les segments
    app.segments.clear();
    // Pour chaque triangle
    for (const Triangle& t : app.triangles)
    {
        // On récupère ses 3 points
        Coords p1 = t.p1;
        Coords p2 = t.p2;
        Coords p3 = t.p3;

        // On met dans xc et yc le centre du cercle circonscrit des points du triangle
        float xc, yc, rsqr;
        CircumCircle(0, 0, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, &xc, &yc, &rsqr);

        // On vérifie les triangles adjacents (ceux qui ont au moins 2 points qui sont similaires au triangle)
        for (const Triangle& other : app.triangles)
        {
            // Vérification des points communs
            int commonPoints = 0;
            if (other.p1.x == p1.x && other.p1.y == p1.y || other.p1.x == p2.x && other.p1.y == p2.y || other.p1.x == p3.x && other.p1.y == p3.y)
                commonPoints++;
            if (other.p2.x == p1.x && other.p2.y == p1.y || other.p2.x == p2.x && other.p2.y == p2.y || other.p2.x == p3.x && other.p2.y == p3.y)
                commonPoints++;
            if (other.p3.x == p1.x && other.p3.y == p1.y || other.p3.x == p2.x && other.p3.y == p2.y || other.p3.x == p3.x && other.p3.y == p3.y)
                commonPoints++;

            // Si le triangle partage au moins 2 points avec le triangle courant, on ajoute un segment dans app.segments
            if (commonPoints >= 2)
            {
                // On met dans xcOther et dans ycOther les coordonnées du centre du cercle circonscrit du triangle adjacent au triangle t
                float xcOther, ycOther, rsqrOther;
                CircumCircle(0, 0, other.p1.x, other.p1.y, other.p2.x, other.p2.y, other.p3.x, other.p3.y, &xcOther, &ycOther, &rsqrOther);

                // Ajout du segment avec les points du centre des cercles circonscrits des deux triangles
                Segment segment(Coords{static_cast<int>(xc), static_cast<int>(yc)}, Coords{static_cast<int>(xcOther), static_cast<int>(ycOther)});
                app.segments.push_back(segment);
            }
        }
    }
}


bool handleEvent(Application &app)
{
    /* Remplissez cette fonction pour gérer les inputs utilisateurs */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        else if (e.type == SDL_WINDOWEVENT_RESIZED)
        {
            app.width = e.window.data1;
            app.height = e.window.data1;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
        }
        else if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                app.focus.x = e.button.x;
                app.focus.y = e.button.y;
                app.points.clear();
            }
            else if (e.button.button == SDL_BUTTON_LEFT)
            {
                app.focus.y = 0;
                app.points.push_back(Coords{e.button.x, e.button.y});
                //fillVectorTriangles(app);
                TriangulationDelaunay(app);
                construitVoronoi(app);
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    SDL_Window *gWindow;
    SDL_Renderer *renderer;
    Application app{720, 720, Coords{0, 0}};
    bool is_running = true;

    // Creation de la fenetre
    gWindow = init("Awesome Voronoi", 720, 720);

    if (!gWindow)
    {
        SDL_Log("Failed to initialize!\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_PRESENTVSYNC

    /*  GAME LOOP  */
    while (true)
    {
        // INPUTS
        is_running = handleEvent(app);
        if (!is_running)
            break;
        // EFFACAGE FRAME
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // DESSIN
        draw(renderer, app);
        // VALIDATION FRAME
        SDL_RenderPresent(renderer);

        // PAUSE en ms
        SDL_Delay(1000 / 30);
    }

    // Free resources and close SDL
    close(gWindow, renderer);

    return 0;
}
