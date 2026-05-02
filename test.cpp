#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

#define MAX_LEVEL 3

class Node {
public:
    int value;
    Node* forward[MAX_LEVEL + 1];

    Node(int val) {
        value = val;
        for (int i = 0; i <= MAX_LEVEL; i++)
            forward[i] = NULL;
    }
};

class SkipList {
    Node* header;
    int level;

public:
    SkipList() {
        level = 0;
        header = new Node(-1); 
    }

    int randomLevel() {
        int lvl = 0;
        while ((rand() % 2) && lvl < MAX_LEVEL)
            lvl++;
        return lvl;
    }

    void insert(int value) {
        Node* update[MAX_LEVEL + 1];
        Node* current = header;

        for (int i = level; i >= 0; i--) {
            while (current->forward[i] != NULL &&
                   current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = current->forward[0];

        if (current == NULL || current->value != value) {
            int rlevel = randomLevel();

            if (rlevel > level) {
                for (int i = level + 1; i <= rlevel; i++)
                    update[i] = header;
                level = rlevel;
            }

            Node* newNode = new Node(value);

            for (int i = 0; i <= rlevel; i++) {
                newNode->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = newNode;
            }
        }
    }

    void search(int target) {
        Node* current = header;

        cout << "\nSearching for " << target << "...\n";

        for (int i = level; i >= 0; i--) {

            while (current->forward[i] != NULL &&
                   current->forward[i]->value < target) {

                cout << "Level " << i << " -> moving from "
                     << current->value << " to "
                     << current->forward[i]->value << endl;

                current = current->forward[i];
            }

            if (current->forward[i] != NULL) {
                cout << "Level " << i << " -> in range between "
                     << current->value << " and "
                     << current->forward[i]->value
                     << ", going down\n";
            }
        }

        current = current->forward[0];

        if (current != NULL && current->value == target)
            cout << "Found: " << target << endl;
        else
            cout << "Not Found\n";
    }
};

int main() {
    srand(time(0));

    SkipList sl;

    int arr[] = {101, 105, 110, 115, 120, 125, 130};

    for (int i = 0; i < 7; i++)
        sl.insert(arr[i]);

    sl.search(125);

    return 0;
}