#include "Enemy/Profile/VisualProfile.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UVisualProfile::IsDataValid(FDataValidationContext& Context) const
{
	if (SkeletalMesh.IsNull())
		Context.AddError(INVTEXT("SkeletalMesh must be assigned"));

	return (Context.GetNumErrors() > 0)
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif
