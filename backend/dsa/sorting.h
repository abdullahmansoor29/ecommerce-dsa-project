#ifndef SORTING_H
#define SORTING_H

#include <vector>
#include "../db/Database.h"   // ProductRow is defined here

// ============================================================
// WHY MERGE SORT (and not std::sort)?
// The course requires manually implementing sorting. Merge Sort
// was chosen because:
// 1) Its time complexity is always O(n log n) — best, worst, and
//    average case. Quicksort's worst case is O(n^2) (e.g. if the
//    data is already sorted).
// 2) It is "stable" — products with equal prices keep their
//    original relative order. This is a nice property for
//    e-commerce (e.g. consistent ordering when two products have
//    the same price).
// TRADE-OFF: Merge Sort uses O(n) extra space (a temporary array),
// whereas Quicksort/Heapsort can sort in-place.
// ============================================================

// Merges two already-sorted halves (left..mid and mid+1..right)
// into one sorted section — the core operation of Merge Sort
void merge(std::vector<ProductRow>& arr, int left, int mid, int right)
{
    std::vector<ProductRow> temp;
    int i = left, j = mid + 1;

    while (i <= mid && j <= right)
    {
        if (arr[i].price <= arr[j].price)
            temp.push_back(arr[i++]);
        else
            temp.push_back(arr[j++]);
    }
    while (i <= mid) temp.push_back(arr[i++]);
    while (j <= right) temp.push_back(arr[j++]);

    for (int k = 0; k < (int)temp.size(); k++)
        arr[left + k] = temp[k];
}

// MERGE SORT: recursively splits the array in half (until only
// one element remains — which is trivially sorted), then merges
// the halves back together in sorted order.
void mergeSort(std::vector<ProductRow>& arr, int left, int right)
{
    if (left >= right) return; // 0 or 1 element — already sorted

    int mid = left + (right - left) / 2;
    mergeSort(arr, left, mid);
    mergeSort(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

// ============================================================
// WHY BINARY SEARCH?
// For the price-range filter (e.g. "products between Rs 100 and
// Rs 500"), we need to find the first element in a sorted array
// whose price is >= minPrice. A linear search would take O(n),
// but since the array is sorted (thanks to Merge Sort), Binary
// Search finds it in O(log n). REQUIREMENT: the array must already
// be sorted, otherwise the result will be incorrect.
// ============================================================
int lowerBound(std::vector<ProductRow>& arr, double minPrice)
{
    int low = 0, high = (int)arr.size();
    while (low < high)
    {
        int mid = low + (high - low) / 2;
        if (arr[mid].price < minPrice)
            low = mid + 1;   // the answer is to the right
        else
            high = mid;      // the answer is here or to the left
    }
    return low;
}

#endif