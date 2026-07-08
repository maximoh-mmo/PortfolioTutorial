#include "UI/OnsetRootLayout.h"
 
UOnsetActivatableWidgetStack* UOnsetRootLayout::GetStackForLayer(EOnsetUILayer Layer) const
{
	switch (Layer)
	{
	case EOnsetUILayer::Game:
		return GameLayerStack;
	case EOnsetUILayer::Menu:
		return MenuLayerStack;
	case EOnsetUILayer::Modal:
		return ModalLayerStack;
	default:
		return nullptr;
	}
}
 